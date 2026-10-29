#!/usr/bin/env python3
"""Build a version-scoped Prism metadata mirror and an exact upstream file manifest.
Defaults to metadata/manifest only. Binary downloads require --download-files.
Python 3.10+ and curl on PATH. Never upload anything to a server.
"""
import argparse, copy, hashlib, json, re, subprocess
from pathlib import Path
from urllib.parse import urlsplit, quote, unquote

META = 'https://meta.prismlauncher.org/v1/'

def encoded(data):
    return (json.dumps(data, ensure_ascii=False, indent=2) + '\n').encode('utf-8')

def sha(data, algorithm='sha256'):
    return hashlib.new(algorithm, data).hexdigest()

def checked_target(root, relative):
    p = (root / relative).resolve()
    if not p.is_relative_to(root.resolve()):
        raise ValueError(f'Unsafe output path: {relative}')
    return p

class Builder:
    def __init__(self, args):
        self.args = args
        self.output = args.output.resolve()
        self.cache = self.output / '_sources'
        self.publish = self.output / 'publish'
        self.files = {}
        self.components = {}
        self.cache.mkdir(parents=True, exist_ok=True)
        self.publish.mkdir(parents=True, exist_ok=True)

    def fetch(self, url, target, expected=None, algorithm='sha1'):
        if target.exists() and (expected is None or sha(target.read_bytes(),algorithm)==expected):
            return target.read_bytes()
        target.parent.mkdir(parents=True, exist_ok=True)
        part=target.with_name(target.name+'.part')
        command=['curl','--fail','--location','--proto','=https','--proto-redir','=https',
                 '--connect-timeout','20','--max-time','1800','--speed-limit','1024','--speed-time','45']
        if self.args.proxy: command += ['--proxy',self.args.proxy]
        command += ['--output',str(part),url]
        subprocess.run(command, check=True)
        data=part.read_bytes()
        if expected and sha(data,algorithm)!=expected:
            raise ValueError(f'Checksum mismatch: {url}')
        part.replace(target)
        return data

    def get_json(self, url):
        name=hashlib.sha256(url.encode()).hexdigest()+'.json'
        return json.loads(self.fetch(url,self.cache/name))

    def register(self, url, digest=None, size=None, path=None):
        parsed=urlsplit(url)
        if parsed.scheme!='https' or not parsed.netloc or parsed.query or parsed.fragment or parsed.username:
            raise ValueError(f'Unsupported source URL (no silent omission): {url}')
        path=path or 'files/'+parsed.netloc+unquote(parsed.path)
        if '..' in Path(path).parts: raise ValueError(f'Unsafe source path: {path}')
        checked_target(self.publish,path)
        record={'source':url,'path':path,'sha1':digest,'size':size}
        if path in self.files:
            old=self.files[path]
            if old!=record: raise ValueError(f'Conflicting output: {path}')
        self.files[path]=record
        return self.args.base_url.rstrip('/')+'/'+quote(path,safe='/')

    def rewrite(self, node):
        if isinstance(node,dict):
            # Every URL in the selected component JSON must be an actual file entry.
            # Stop on unknown metadata formats instead of producing a partial mirror.
            if 'url' in node:
                url=node['url']
                if not url: return
                if not node.get('sha1') or 'size' not in node:
                    raise ValueError(f'URL without a supported hash/size entry: {url}')
                node['url']=self.register(url,node['sha1'],node['size'])
            for value in node.values(): self.rewrite(value)
        elif isinstance(node,list):
            for value in node:self.rewrite(value)

    def component(self, uid, version):
        key=(uid,version)
        if key in self.components:return
        raw=self.get_json(META+uid+'/'+version+'.json')
        if raw.get('uid')!=uid or raw.get('version')!=version: raise ValueError('Component identity mismatch')
        self.components[key]=raw
        for req in raw.get('requires',[]):
            selected=req.get('equals') or req.get('suggests')
            if not selected: raise ValueError(f'Unpinned dependency: {req}')
            self.component(req['uid'],selected)
        asset=raw.get('assetIndex')
        if asset:
            data=self.fetch(asset['url'],self.cache/(asset['sha1']+'.json'),asset['sha1'])
            index=json.loads(data)
            for obj in index['objects'].values():
                h=obj['hash']; rel=h[:2]+'/'+h
                self.register('https://resources.download.minecraft.net/'+rel,h,obj['size'],'assets/'+rel)
            # The unmodified index is ready to publish; its original SHA1 stays valid.
            local='files/'+urlsplit(asset['url']).netloc+urlsplit(asset['url']).path
            p=checked_target(self.publish,local);p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)

    def build(self):
        # A previous successful run must not certify a new or interrupted build.
        (self.output/'download-complete.json').unlink(missing_ok=True)
        self.component('net.minecraft',self.args.minecraft)
        self.component('net.minecraftforge',self.args.forge)
        packages=[]
        by_uid={}
        for (uid,version),raw in self.components.items():
            rewritten=copy.deepcopy(raw)
            self.rewrite(rewritten)
            data=encoded(rewritten)
            p=checked_target(self.publish,f'meta/v1/{uid}/{version}.json');p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
            item={k:raw[k] for k in ['version','releaseTime','type','requires','conflicts','volatile'] if k in raw}
            item.update(sha256=sha(data),recommended=True)
            by_uid.setdefault(uid,{'formatVersion':1,'uid':uid,'name':raw['name'],'versions':[]})['versions'].append(item)
        for uid,listing in by_uid.items():
            data=encoded(listing);(self.publish/f'meta/v1/{uid}/index.json').write_bytes(data)
            packages.append({'uid':uid,'name':listing['name'],'sha256':sha(data)})
        (self.publish/'meta/v1/index.json').write_bytes(encoded({'formatVersion':1,'packages':packages}))
        (self.publish/'packs').mkdir(exist_ok=True)
        records=sorted(self.files.values(),key=lambda x:x['path'])
        manifest={'minecraft':self.args.minecraft,'forge':self.args.forge,'base_url':self.args.base_url,
                  'components':[{'uid':u,'version':v} for u,v in self.components],
                  'file_count':len(records),'total_bytes':sum(x['size'] or 0 for x in records),
                  'files':records}
        (self.output/'download-manifest.json').write_bytes(encoded(manifest))
        (self.output/'official-sources.txt').write_text('\n'.join(x['source'] for x in records)+'\n',encoding='utf-8')
        print(json.dumps({k:v for k,v in manifest.items() if k!='files'},ensure_ascii=False,indent=2))
        if self.args.download_files:
            if '.invalid' in self.args.base_url: raise ValueError('Use the real public base URL before downloading/publishing')
            for i,item in enumerate(records,1):
                print(f'[{i}/{len(records)}] {item["path"]}',flush=True)
                data = self.fetch(item['source'],checked_target(self.publish,item['path']),item['sha1'])
                if len(data) != item['size']:
                    raise ValueError(f'Unexpected size: {item["source"]}')
            (self.output/'download-complete.json').write_bytes(encoded({'files':len(records),'base_url':self.args.base_url}))
        print('Publish directory:',self.publish)
        print('Metadata prepared; binary files '+('downloaded.' if self.args.download_files else 'NOT downloaded.'))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--base-url',required=True,help='HTTPS public root mapped to publish/, e.g. https://server/niijima/')
    p.add_argument('--minecraft',default='1.20.1');p.add_argument('--forge',default='47.4.6')
    p.add_argument('--proxy');p.add_argument('--download-files',action='store_true')
    args=p.parse_args()
    root=urlsplit(args.base_url)
    if root.scheme!='https' or not root.hostname or root.query or root.fragment or root.username:p.error('base-url must be an HTTPS root URL')
    Builder(args).build()
