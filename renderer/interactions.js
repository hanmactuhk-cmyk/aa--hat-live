'use strict';
(() => {
 const $=id=>document.getElementById(id);const tr=s=>window.hnLocale.translate(s);let loading=false;
 function render(data){if(!data)return;data.names.forEach((name,i)=>{const b=document.querySelector(`[data-sfx="${i}"]`);if(b)b.textContent=tr(name);});$('sfxVolume').value=data.volume;}
 async function action(pad){if(loading)return;loading=true;try{await window.hnEnsureAudio();await window.studio.command('soundboard',{action:'trigger',pad});const b=document.querySelector(`[data-sfx="${pad}"]`);b?.classList.add('firing');setTimeout(()=>b?.classList.remove('firing'),650);}catch(e){$('status').textContent=e.message;}finally{loading=false;}}
 document.querySelectorAll('[data-sfx]').forEach(b=>b.onclick=()=>action(Number(b.dataset.sfx)));
 $('sfxStop').onclick=()=>window.studio.command('soundboard',{action:'stop'}).catch(e=>$('status').textContent=e.message);
 $('sfxVolume').onchange=()=>window.studio.command('soundboard',{action:'volume',volume:Number($('sfxVolume').value)}).catch(e=>$('status').textContent=e.message);
 $('sfxLoad').onclick=async()=>{try{render(await window.studio.loadSound(Number($('sfxPad').value)));}catch(e){$('status').textContent=e.message;}};
 const reduced=matchMedia('(prefers-reduced-motion: reduce)').matches;
 if(!reduced){const splash=document.createElement('div');splash.className='boot-splash';splash.innerHTML='<img src="assets/logo.webp" alt="HNStudio Musik AI"><span>LIVE · RECORD · CREATE</span>';document.body.append(splash);setTimeout(()=>{splash.classList.add('depart');setTimeout(()=>splash.remove(),350);},400);}
 document.addEventListener('click',e=>{if(reduced)return;const button=e.target.closest('button');if(!button||button.disabled)return;const wave=document.createElement('span');wave.className='click-ripple';const rect=button.getBoundingClientRect();wave.style.left=`${e.clientX?e.clientX-rect.left:rect.width/2}px`;wave.style.top=`${e.clientY?e.clientY-rect.top:rect.height/2}px`;button.append(wave);setTimeout(()=>wave.remove(),500);});
 window.studio.onEvent(data=>{if(data.event==='ready')window.studio.command('soundboard',{action:'state'}).then(render).catch(()=>{});});
})();
