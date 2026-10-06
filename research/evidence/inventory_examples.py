import zipfile, pathlib, hashlib, json, collections, xml.etree.ElementTree as E
base=pathlib.Path(r'C:\Users\Dell4\AppData\Local\SimPHY\app\libs\simulations')
out=pathlib.Path(r'C:\Users\Dell4\simphy-decomp\research\evidence')
manifest=[]; paths={}; types=collections.defaultdict(set)
for p in sorted(base.rglob('*.ssim')):
 r={'source':str(p.relative_to(base)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'size':p.stat().st_size,'magic':p.read_bytes()[:8].hex()}
 try:
  with zipfile.ZipFile(p) as z:
   r['entries']=[{'name':i.filename,'bytes':i.file_size,'compression':i.compress_type} for i in z.infolist()]
   data=z.read('simulation.xml'); root=E.fromstring(data);r['root_attributes']=root.attrib
   def walk(e,path):
    path=path+'/'+e.tag
    d=paths.setdefault(path,{'count':0,'sources':set(),'attributes':{},'values':set()});d['count']+=1;d['sources'].add(r['source'])
    for k,v in e.attrib.items():
     d['attributes'].setdefault(k,set()).add(v if len(v)<150 else '<long value>')
     if k.lower()=='type':types[path].add(v)
    if not len(e) and e.text and len(e.text.strip())<120:d['values'].add(e.text.strip())
    for c in e:walk(c,path)
   walk(root,'')
 except Exception as ex:r['error']=str(ex)
 manifest.append(r)
def conv(x):return sorted(x) if isinstance(x,set) else str(x)
(out/'archive_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
(out/'xml_inventory.json').write_text(json.dumps(paths,default=conv,indent=2),encoding='utf-8')
print('Archives',len(manifest),'XML paths',len(paths),'errors',[(r['source'],r['error']) for r in manifest if 'error' in r])
print(json.dumps(types,default=conv,indent=2))
