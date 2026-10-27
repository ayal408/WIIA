(()=>{'use strict';
 const pairs=[['q','/'],['w',"'"],['e','ק'],['r','ר'],['t','א'],['y','ט'],['u','ו'],['i','ן'],['o','ם'],['p','פ'],['[',']'],[']','['],['a','ש'],['s','ד'],['d','ג'],['f','כ'],['g','ע'],['h','י'],['j','ח'],['k','ל'],['l','ך'],[';','ף'],["'",','],['z','ז'],['x','ס'],['c','ב'],['v','ה'],['b','נ'],['n','מ'],['m','צ'],[',','ת'],['.','ץ'],['/','.']];
 const en=new Map(pairs),he=new Map(pairs.map(([a,b])=>[b,a]));
 function convert(text){const heb=(text.match(/[א-ת]/g)||[]).length,eng=(text.match(/[a-z]/gi)||[]).length,map=heb>eng?he:en;return [...text].map(c=>map.get(map===en?c.toLowerCase():c)||c).join('');}
 function replaceSelection(target){
  if(!target||target.disabled||target.readOnly)return false;
  if(target.tagName==='TEXTAREA'||(target.tagName==='INPUT'&&['text','search'].includes(target.type))){const a=target.selectionStart,b=target.selectionEnd;if(a===null||b===null||a===b)return false;const text=convert(target.value.slice(a,b));target.setRangeText(text,a,b,'select');target.dispatchEvent(new Event('input',{bubbles:true}));return true;}
  const editor=target.closest?.('[contenteditable="true"]');if(!editor)return false;const selection=window.getSelection();if(!editor||!selection?.rangeCount||selection.isCollapsed)return false;const range=selection.getRangeAt(0);if(!editor.contains(range.startContainer)||!editor.contains(range.endContainer))return false;return document.execCommand('insertText',false,convert(selection.toString()));
 }
 globalThis.IliMailKeyboard={convert,replaceSelection};if(typeof module==='object'&&module.exports)module.exports={convert};
})();
