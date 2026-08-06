const games = [
  ['Forza Horizon','Corrida · 2012','#ff8533','#441714',true],['Halo 3','Ação · 2007','#426d48','#101d16',true],
  ['Gears of War 3','Ação · 2011','#c59d57','#2f2116',false],['Red Dead Redemption','Aventura · 2010','#b12621','#35100e',true],
  ['Skate 3','Esporte · 2010','#e4d3a5','#46748a',false],['Minecraft','Sandbox · 2012','#65a84b','#254a20',false],
  ['Fable II','RPG · 2008','#54798c','#14242d',false],['Alan Wake','Suspense · 2010','#64737c','#171c20',true],
  ['Portal 2','Puzzle · 2011','#dbe0d8','#727e83',false],['Mass Effect 2','RPG · 2010','#387293','#101d2e',false],
  ['Assassin’s Creed II','Aventura · 2009','#d7dad4','#474b48',false],['Burnout Paradise','Corrida · 2008','#e76824','#3a1710',false]
].map((g,i)=>({name:g[0],meta:g[1],colors:[g[2],g[3]],favorite:g[4],played:12-i,size:4+(i*1.7),installed:i!==10}));

const savedFavorites = JSON.parse(localStorage.getItem('aurorax-favorites') || '[]');
games.forEach(game => { if (savedFavorites.includes(game.name)) game.favorite = true; });
let activeFilter = 'all';

const generic = {
  apps:['⌘','APLICATIVOS','Homebrew & utilitários','Gerencie ferramentas instaladas, players e serviços.',[['Aurora Updater','Atualizações seguras'],['FTP Server','Transferência pela rede'],['Media Player','Vídeos, música e fotos']]],
  emulators:['◈','EMULADORES','8 sistemas disponíveis','Organize ROMs e configure controles por sistema.',[['Xbox Classic','Compatibilidade'],['Arcade','Coleção retrô'],['Gerenciar ROMs','Capas e favoritos']]],
  files:['▱','ARQUIVOS','Gerenciador de arquivos','Acesse dispositivos locais e compartilhamentos de rede.',[['HDD1:','68% utilizado'],['USB0:','Dispositivo conectado'],['REDE','SMB e FTP']]],
  system:['⚙','SISTEMA','Status & configurações','Monitore o console e personalize sua experiência.',[['CPU 52°','Temperatura normal'],['GPU 55°','Temperatura normal'],['REDE','192.168.1.36']]]
};

const views=document.querySelectorAll('.view');
function show(id){views.forEach(v=>v.classList.remove('active'));if(id==='games')document.querySelector('#games').classList.add('active');else if(id==='home')document.querySelector('#home').classList.add('active');else{const d=generic[id];document.querySelector('#genericIcon').textContent=d[0];document.querySelector('#genericKicker').textContent=d[1];document.querySelector('#genericTitle').textContent=d[2];document.querySelector('#genericText').textContent=d[3];document.querySelector('#quickGrid').innerHTML=d[4].map(x=>`<button><b>${x[0]}</b><small>${x[1]}</small></button>`).join('');document.querySelector('#generic').classList.add('active')}window.scrollTo(0,0)}
document.querySelectorAll('[data-view]').forEach(b=>b.addEventListener('click',()=>show(b.dataset.view)));

function render(filter=activeFilter){
  activeFilter=filter;
  const query=document.querySelector('#search').value.toLowerCase();
  let list=games.filter(g=>g.name.toLowerCase().includes(query) && (filter!=='favorite'||g.favorite) && (filter!=='recent'||g.played>7) && (filter!=='installed'||g.installed));
  const sort=document.querySelector('#sort').value;
  if(sort==='played')list.sort((a,b)=>b.played-a.played);else if(sort==='size')list.sort((a,b)=>b.size-a.size);else list.sort((a,b)=>a.name.localeCompare(b.name));
  document.querySelector('#allCount').textContent=games.length;
  document.querySelector('#favoriteCount').textContent=games.filter(g=>g.favorite).length;
  document.querySelector('#gameGrid').innerHTML=list.map(g=>`<button class="game-card focusable" data-game="${games.indexOf(g)}"><div class="cover" style="background:linear-gradient(145deg,${g.colors[0]},${g.colors[1]})"><strong>${g.name.toUpperCase()}</strong></div><strong>${g.favorite?'♥ ':''}${g.name}</strong><small>${g.meta} · ${g.size.toFixed(1)} GB ${g.installed?'':'· não instalado'}</small></button>`).join('') || '<p class="muted">Nenhum título corresponde ao filtro.</p>';
  document.querySelectorAll('[data-game]').forEach(c=>c.onclick=()=>openGame(+c.dataset.game));
}
function persist(){localStorage.setItem('aurorax-favorites',JSON.stringify(games.filter(g=>g.favorite).map(g=>g.name)))}
function openGame(i){const g=games[i];document.querySelector('#modalTitle').textContent=g.name;document.querySelector('#modalMeta').textContent=`${g.meta} · ${g.size.toFixed(1)} GB · ${g.installed?'Instalado em HDD1':'Disponível para instalar'}`;document.querySelector('#modalCover').style.background=`linear-gradient(145deg,${g.colors[0]},${g.colors[1]})`;document.querySelector('#favorite').textContent=g.favorite?'♥ FAVORITO':'♡ FAVORITO';document.querySelector('#favorite').onclick=()=>{g.favorite=!g.favorite;persist();openGame(i);render()};document.querySelector('#modal').classList.add('open');document.querySelector('#modal').setAttribute('aria-hidden','false')}
document.querySelector('.modal-close').onclick=()=>document.querySelector('#modal').classList.remove('open');
document.querySelector('#search').oninput=()=>render();document.querySelector('#sort').onchange=()=>render();
document.querySelectorAll('[data-filter]').forEach(b=>b.onclick=()=>{document.querySelectorAll('[data-filter]').forEach(x=>x.classList.remove('selected'));b.classList.add('selected');render(b.dataset.filter)});
function toast(text){const t=document.querySelector('#toast');t.textContent=text;t.classList.add('show');setTimeout(()=>t.classList.remove('show'),2200)}
document.addEventListener('click',e=>{const b=e.target.closest('[data-toast]');if(b)toast(b.dataset.toast)});
const guide=document.querySelector('#guide');
function toggleGuide(force){const open=force??!guide.classList.contains('open');guide.classList.toggle('open',open);guide.setAttribute('aria-hidden',String(!open))}
document.querySelector('#closeGuide').onclick=()=>toggleGuide(false);
document.querySelectorAll('#guide [data-view]').forEach(b=>b.addEventListener('click',()=>toggleGuide(false)));
document.querySelector('[data-guide="downloads"]').onclick=()=>toast('2 downloads na fila · 38% concluído');
document.querySelector('[data-guide="network"]').onclick=()=>toast('FTP ativo em 192.168.1.36:21');
document.querySelector('[data-guide="power"]').onclick=()=>toast('Menu de energia protegido por confirmação');
document.querySelector('[data-guide="settings"]').onclick=()=>{toggleGuide(false);document.querySelector('#settingsPanel').classList.add('open')};
document.querySelector('.settings-close').onclick=()=>document.querySelector('#settingsPanel').classList.remove('open');
const settings=JSON.parse(localStorage.getItem('aurorax-settings')||'{}');
if(settings.accent)document.documentElement.style.setProperty('--green',settings.accent);
if(settings.profile)document.querySelector('.avatar').textContent=settings.profile;
if(settings.reduceMotion)document.body.classList.add('reduce-motion');
document.querySelector('#accent').value=settings.accent||'#78e819';document.querySelector('#profileName').value=settings.profile||'AX';document.querySelector('#reduceMotion').checked=!!settings.reduceMotion;document.querySelector('#sound').checked=settings.sound!==false;
document.querySelector('#saveSettings').onclick=()=>{const value={accent:document.querySelector('#accent').value,profile:document.querySelector('#profileName').value||'AX',reduceMotion:document.querySelector('#reduceMotion').checked,sound:document.querySelector('#sound').checked};localStorage.setItem('aurorax-settings',JSON.stringify(value));document.documentElement.style.setProperty('--green',value.accent);document.querySelector('.avatar').textContent=value.profile;document.body.classList.toggle('reduce-motion',value.reduceMotion);document.querySelector('#settingsPanel').classList.remove('open');toast('Preferências salvas')};

document.addEventListener('keydown',e=>{
  const key=e.key.toLowerCase();
  if(key==='g'||e.key==='Tab'){e.preventDefault();toggleGuide();return}
  if(e.key==='Escape'||key==='b'){document.querySelector('#modal').classList.remove('open');document.querySelector('#settingsPanel').classList.remove('open');if(guide.classList.contains('open'))toggleGuide(false);else show('home')}
  if(key==='y')toast('Biblioteca atualizada · capas sincronizadas');
  if(key==='x'){const card=document.activeElement.closest?.('[data-game]');if(card){games[+card.dataset.game].favorite=!games[+card.dataset.game].favorite;persist();render();}}
  if(e.key==='Enter'&&document.activeElement.matches('button'))document.activeElement.click();
});
setInterval(()=>{const now=new Date();document.querySelector('#clock').textContent=now.toLocaleTimeString('pt-BR',{hour:'2-digit',minute:'2-digit'});document.querySelector('#guideTime').textContent=`Sessão ativa · ${now.toLocaleDateString('pt-BR')}`},1000);render();
