const games = [
  ['Forza Horizon','Corrida · 2012','#ff8533','#441714',true],['Halo 3','Ação · 2007','#426d48','#101d16',true],
  ['Gears of War 3','Ação · 2011','#c59d57','#2f2116',false],['Red Dead Redemption','Aventura · 2010','#b12621','#35100e',true],
  ['Skate 3','Esporte · 2010','#e4d3a5','#46748a',false],['Minecraft','Sandbox · 2012','#65a84b','#254a20',false],
  ['Fable II','RPG · 2008','#54798c','#14242d',false],['Alan Wake','Suspense · 2010','#64737c','#171c20',true],
  ['Portal 2','Puzzle · 2011','#dbe0d8','#727e83',false],['Mass Effect 2','RPG · 2010','#387293','#101d2e',false],
  ['Assassin’s Creed II','Aventura · 2009','#d7dad4','#474b48',false],['Burnout Paradise','Corrida · 2008','#e76824','#3a1710',false]
  ,['Geometry Wars 2','Arcade · 2008','#ec4af0','#241044',true],['Trials Evolution','Arcade · 2012','#f08728','#33200f',false],
  ['Kinect Sports','Kinect · 2010','#64bfe5','#173b55',false],['Dance Central 3','Kinect · 2012','#ec4c87','#4c1230',false],
  ['XeXMenu','Homebrew · Utilitário','#63df8c','#143b28',true],['RetroArch 360','Homebrew · Emulador','#6f70df','#202153',false],
  ['Media Center','Aplicativo · Mídia','#42c9c4','#103b3a',false],['File Manager','Aplicativo · Sistema','#d4a944','#49350f',false]
].map((g,i)=>({name:g[0],meta:g[1],colors:[g[2],g[3]],favorite:g[4],played:20-i,size:Math.max(.2,4+(i*1.7)),installed:i!==10,platform:i<12?'xbox360':i<14?'arcade':i<16?'kinect':i<18?'homebrew':'apps'}));

const savedFavorites = JSON.parse(localStorage.getItem('aurorax-favorites') || '[]');
games.forEach(game => { if (savedFavorites.includes(game.name)) game.favorite = true; });
let activeFilter = 'all';
let activePlatform = 'all';

const generic = {
  apps:['⌘','APLICATIVOS','Homebrew & utilitários','Gerencie ferramentas instaladas, players e serviços.',[['Aurora Updater','Atualizações seguras'],['FTP Server','Transferência pela rede'],['Media Player','Vídeos, música e fotos']]],
  emulators:['◈','EMULADORES','8 sistemas disponíveis','Organize ROMs e configure controles por sistema.',[['Xbox Classic','Compatibilidade'],['Arcade','Coleção retrô'],['Gerenciar ROMs','Capas e favoritos']]],
  files:['▱','ARQUIVOS','Gerenciador de arquivos','Acesse dispositivos locais e compartilhamentos de rede.',[['HDD1:','68% utilizado'],['USB0:','Dispositivo conectado'],['REDE','SMB e FTP']]],
  system:['⚙','SISTEMA','Status & configurações','Monitore o console e personalize sua experiência.',[['CPU 52°','Temperatura normal'],['GPU 55°','Temperatura normal'],['REDE','192.168.1.36']]]
};

const views=document.querySelectorAll('.view');
function show(id){views.forEach(v=>v.classList.remove('active'));const direct=document.querySelector(`#${id}.view`);if(direct)direct.classList.add('active');else{const d=generic[id];document.querySelector('#genericIcon').textContent=d[0];document.querySelector('#genericKicker').textContent=d[1];document.querySelector('#genericTitle').textContent=d[2];document.querySelector('#genericText').textContent=d[3];document.querySelector('#quickGrid').innerHTML=d[4].map(x=>`<button><b>${x[0]}</b><small>${x[1]}</small></button>`).join('');document.querySelector('#generic').classList.add('active')}window.scrollTo(0,0)}
document.querySelectorAll('[data-view]').forEach(b=>b.addEventListener('click',()=>show(b.dataset.view)));
document.querySelectorAll('.category-bar [data-platform]').forEach(b=>b.addEventListener('click',()=>{activePlatform=b.dataset.platform;syncPlatformTabs();render()}));

function render(filter=activeFilter){
  activeFilter=filter;
  const query=document.querySelector('#search').value.toLowerCase();
  let list=games.filter(g=>g.name.toLowerCase().includes(query) && (activePlatform==='all'||g.platform===activePlatform) && (filter!=='favorite'||g.favorite) && (filter!=='recent'||g.played>14) && (filter!=='installed'||g.installed));
  const sort=document.querySelector('#sort').value;
  if(sort==='played')list.sort((a,b)=>b.played-a.played);else if(sort==='size')list.sort((a,b)=>b.size-a.size);else list.sort((a,b)=>a.name.localeCompare(b.name));
  document.querySelector('#allCount').textContent=games.length;
  document.querySelector('#favoriteCount').textContent=games.filter(g=>g.favorite).length;
  document.querySelector('#gameGrid').innerHTML=list.map(g=>`<button class="game-card focusable" data-game="${games.indexOf(g)}"><div class="cover" style="background:linear-gradient(145deg,${g.colors[0]},${g.colors[1]})"><strong>${g.name.toUpperCase()}</strong></div><strong>${g.favorite?'♥ ':''}${g.name}</strong><small>${g.meta} · ${g.size.toFixed(1)} GB ${g.installed?'':'· não instalado'}</small></button>`).join('') || '<p class="muted">Nenhum título corresponde ao filtro.</p>';
  document.querySelectorAll('[data-game]').forEach(c=>c.onclick=()=>openGame(+c.dataset.game));
}
function syncPlatformTabs(){document.querySelectorAll('.platform-tabs [data-platform]').forEach(b=>b.classList.toggle('selected',b.dataset.platform===activePlatform))}
document.querySelectorAll('.platform-tabs [data-platform]').forEach(b=>b.onclick=()=>{activePlatform=b.dataset.platform;syncPlatformTabs();render()});
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
const settingIds=['accent','profileName','reduceMotion','sound','weatherLocation','temperatureUnit','libraryLayout','autoFtp','autoUpdate','confirmLaunch'];
settingIds.forEach(id=>{const el=document.querySelector(`#${id}`);const key=id==='profileName'?'profile':id;if(settings[key]!==undefined){if(el.type==='checkbox')el.checked=settings[key];else el.value=settings[key]}});
document.querySelector('#accent').value=settings.accent||'#78e819';document.querySelector('#profileName').value=settings.profile||'AX';document.querySelector('#reduceMotion').checked=!!settings.reduceMotion;document.querySelector('#sound').checked=settings.sound!==false;
document.querySelector('#saveSettings').onclick=()=>{const value={};settingIds.forEach(id=>{const el=document.querySelector(`#${id}`);value[id==='profileName'?'profile':id]=el.type==='checkbox'?el.checked:el.value});localStorage.setItem('aurorax-settings',JSON.stringify(value));document.documentElement.style.setProperty('--green',value.accent);document.querySelector('.avatar').textContent=value.profile;document.body.classList.toggle('reduce-motion',value.reduceMotion);document.querySelector('#gameGrid').classList.toggle('compact',value.libraryLayout==='compact');updateWeather(value.weatherLocation,value.temperatureUnit);document.querySelector('#settingsPanel').classList.remove('open');toast('Todas as configurações foram salvas')};

const storeItems=[['Tema Blades','Tema','Uma releitura leve da interface clássica','#6edb49'],['Pacote PT-BR','Tradução','Textos revisados para a dashboard','#31a7df'],['Capas essenciais','Capas','Metadados e artes da biblioteca','#ef9034'],['Aurora Media','Aplicativo','Player de mídia da comunidade','#b365e7'],['Retro Pack','Emulador','Configurações e artes retrô','#e74f66'],['Tema Carbon','Tema','Visual escuro de alto contraste','#748079']];
document.querySelector('#storeGrid').innerHTML=storeItems.map((item,i)=>`<article class="store-item"><div style="--item:${item[3]}">${i%2?'◈':'✦'}</div><p class="eyebrow">${item[1]}</p><h3>${item[0]}</h3><p>${item[2]}</p><button data-package="${item[0]}">${localStorage.getItem(`package-${item[0]}`)?'✓ INSTALADO':'⇩ INSTALAR'}</button></article>`).join('');
document.querySelectorAll('[data-package]').forEach(b=>b.onclick=()=>{localStorage.setItem(`package-${b.dataset.package}`,'installed');b.textContent='✓ INSTALADO';toast(`${b.dataset.package} instalado com sucesso`)});

const climates={'São Paulo':[24,'☀','Ensolarado'],'Rio de Janeiro':[28,'☀','Céu limpo'],'Curitiba':[17,'☁','Nublado'],'Brasília':[26,'⛅','Parcialmente nublado'],'Lisboa':[20,'🌤','Agradável']};
function updateWeather(city=settings.weatherLocation||'São Paulo',unit=settings.temperatureUnit||'c'){const data=climates[city]||climates['São Paulo'];const temp=unit==='f'?Math.round(data[0]*9/5+32):data[0];const suffix=unit==='f'?'°F':'°';document.querySelector('#weatherCity').textContent=city;document.querySelector('#weatherTitle').textContent=city;document.querySelector('#weatherTemp').textContent=temp+suffix;document.querySelector('#weatherLarge').textContent=temp+suffix;document.querySelector('#weatherIcon').textContent=data[1];document.querySelector('#weatherCondition').textContent=data[2];document.querySelector('#forecast').innerHTML=['HOJE','AMANHÃ','SÁBADO','DOMINGO','SEGUNDA'].map((day,i)=>`<div><small>${day}</small><b>${i%2?'⛅':data[1]}</b><strong>${temp-i}${suffix}</strong></div>`).join('')}
updateWeather();

document.addEventListener('keydown',e=>{
  const key=e.key.toLowerCase();
  if(key==='g'||e.key==='Tab'){e.preventDefault();toggleGuide();return}
  if(e.key==='Escape'||key==='b'){document.querySelector('#modal').classList.remove('open');document.querySelector('#settingsPanel').classList.remove('open');if(guide.classList.contains('open'))toggleGuide(false);else show('home')}
  if(key==='y')toast('Biblioteca atualizada · capas sincronizadas');
  if(key==='x'){const card=document.activeElement.closest?.('[data-game]');if(card){games[+card.dataset.game].favorite=!games[+card.dataset.game].favorite;persist();render();}}
  if(e.key==='Enter'&&document.activeElement.matches('button'))document.activeElement.click();
});
setInterval(()=>{const now=new Date();document.querySelector('#clock').textContent=now.toLocaleTimeString('pt-BR',{hour:'2-digit',minute:'2-digit'});document.querySelector('#guideTime').textContent=`Sessão ativa · ${now.toLocaleDateString('pt-BR')}`},1000);render();
