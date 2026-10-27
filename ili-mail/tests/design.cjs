const {test}=require('node:test');const assert=require('node:assert/strict');const fs=require('node:fs');const vm=require('node:vm');const path=require('node:path');
// Minimal DOM fixture. This checks state and text handling, not browser layout or Gmail compatibility.
class Element{
 constructor(tag='div'){this.tag=tag;this.children=[];this.hidden=false;this.value='';this.checked=false;this.textContent='';this.attrs={};this.className='';this.style={setProperty(){}};}
 append(...e){this.children.push(...e);}replaceChildren(...e){this.children=e;}setAttribute(k,v){this.attrs[k]=v;}getAttribute(k){return this.attrs[k]??null;}focus(){this.focused=true;}addEventListener(){}checkValidity(){return true;}
 attachShadow(){this.shadow=new Element('shadow');this.shadow.nodes={};this.shadow.querySelector=s=>this.shadow.nodes[s]||null;Object.defineProperty(this.shadow,'innerHTML',{set:html=>{for(const [,id]of html.matchAll(/id="([^"]+)"/g)){const el=new Element();el.hidden=['drawer','updated','older','settings-page'].includes(id);this.shadow.nodes['#'+id]=el;}}});return this.shadow;}
}
function setup(){
 const html=new Element('html'),body=new Element('body'),events={};let data={},changeListener;let rows=[];let copied='';const source={querySelectorAll:()=>rows};
 const document={documentElement:html,body,activeElement:null,createElement:t=>new Element(t),querySelector:s=>s==='[role="main"]'?source:s==='[role="main"] h2.hP'?{textContent:'בדיקת שיחה'}:null,querySelectorAll:()=>[]};
 const window={addEventListener:(k,f)=>events[k]=f};const location={href:'https://mail.google.com/mail/u/0/#inbox/one'};
 const chrome={storage:{local:{get:async defaults=>({...defaults,...data}),set:async o=>{Object.assign(data,o);}},onChanged:{addListener:f=>changeListener=f}}};
 const context={window,document,chrome,location,MutationObserver:class{observe(){}disconnect(){}},navigator:{clipboard:{writeText:async t=>copied=t}},setTimeout,clearTimeout};
 vm.runInNewContext(fs.readFileSync(path.join(__dirname,'../design.js'),'utf8'),context);
 const host=html.children.find(e=>e.id==='ili-mail-design'),style=html.children.find(e=>e.id==='ili-mail-theme'),nodes=host.shadow.nodes;
 return {window,location,events,style,nodes,source,setRows:items=>{rows=items.map((m,i)=>({innerText:m.text,getClientRects:()=>m.hidden?[]:[{}],closest:()=>({querySelector:s=>s==='.gD[email]'?{textContent:m.sender||'Sender',getAttribute:k=>k==='email'?m.email||'sender@example.com':null}:s==='.g3'?{textContent:'27/10',getAttribute:()=>null}:null})}));},change:o=>{for(const [k,v]of Object.entries(o))data[k]=v;changeListener(Object.fromEntries(Object.entries(o).map(([k,v])=>[k,{newValue:v}])),'local');},getCopied:()=>copied,getData:()=>data};
}
test('chat excludes hidden bodies, treats message markup as text, filters, and expands older messages',async()=>{
 const app=setup();await Promise.resolve();app.setRows([...Array.from({length:7},(_,i)=>({text:'message '+i})),{text:'secret',hidden:true},{text:'<img src=x onerror=alert(1)>',sender:'<b>sender</b>'}]);app.window.IliMailDesign.open('chat');
 const n=app.nodes;assert.equal(n['#messages'].children.length,5);assert.equal(n['#older'].hidden,false);n['#older'].onclick();assert.equal(n['#messages'].children.length,8);const last=n['#messages'].children.at(-1);assert.equal(last.children[0].textContent,'<b>sender</b> · sender@example.com');assert.equal(last.children[2].textContent,'<img src=x onerror=alert(1)>');assert.equal(last.children[2].children.length,0);
 n['#search'].value='message 2';n['#search'].oninput();assert.equal(n['#messages'].children.length,1);await n['#copy'].onclick();assert.ok(!app.getCopied().includes('secret'));assert.ok(app.getCopied().includes('message 0'));
});
test('appearance updates live, original reset removes all Gmail CSS, own identity is explicit',async()=>{
 const app=setup();await Promise.resolve();assert.equal(app.style.textContent,'');app.change({theme:'ocean',density:'compact',rounded:true});assert.ok(app.style.textContent.includes('#edf5fd'));assert.ok(app.style.textContent.includes('height:32px'));assert.ok(app.style.textContent.includes('border-radius'));
 app.setRows([{text:'hello',email:'me@example.com'}]);app.window.IliMailDesign.open('chat');assert.equal(app.nodes['#messages'].children[0].className,'');app.change({ownEmail:'me@example.com'});assert.equal(app.nodes['#messages'].children[0].className,'mine');await app.nodes['#reset'].onclick();assert.equal(app.style.textContent,'');assert.equal(app.getData().ownEmail,'me@example.com');
});
test('long bodies collapse, navigation clears stale conversation, close restores focus',async()=>{
 const app=setup();await Promise.resolve();app.setRows([{text:'x'.repeat(1200)}]);const trigger=new Element('button');app.window.IliMailDesign.open('chat',trigger);assert.equal(app.nodes['#messages'].children[0].children.at(-1).tag,'details');app.setRows([]);app.location.href='#inbox';app.events.hashchange();assert.equal(app.nodes['#messages'].children[0].className,'empty');app.nodes['#close'].onclick();assert.equal(app.nodes['#drawer'].hidden,true);assert.equal(trigger.focused,true);
});
