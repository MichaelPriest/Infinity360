const games = [
  ['Forza Horizon','Corrida · 2012','#ff8533','#441714',true],['Halo 3','Ação · 2007','#426d48','#101d16',true],
  ['Gears of War 3','Ação · 2011','#c59d57','#2f2116',true],['Red Dead Redemption','Aventura · 2010','#b12621','#35100e',true],
  ['Skate 3','Esporte · 2010','#e4d3a5','#46748a',false],['Minecraft','Sandbox · 2012','#65a84b','#254a20',false],
  ['Fable II','RPG · 2008','#54798c','#14242d',false],['Alan Wake','Suspense · 2010','#64737c','#171c20',true],
  ['Portal 2','Puzzle · 2011','#dbe0d8','#727e83',false],['Mass Effect 2','RPG · 2010','#387293','#101d2e',false],
  ['Assassin’s Creed II','Aventura · 2009','#d7dad4','#474b48',false],['Burnout Paradise','Corrida · 2008','#e76824','#3a1710',false]
  ,['Geometry Wars 2','Arcade · 2008','#ec4af0','#241044',true],['Trials Evolution','Arcade · 2012','#f08728','#33200f',false],
  ['Kinect Sports','Kinect · 2010','#64bfe5','#173b55',false],['Dance Central 3','Kinect · 2012','#ec4c87','#4c1230',false],
  ['XeXMenu','Homebrew · Utilitário','#63df8c','#143b28',true],['RetroArch 360','Homebrew · Emulador','#6f70df','#202153',false],
  ['Media Center','Aplicativo · Mídia','#42c9c4','#103b3a',false],['File Manager','Aplicativo · Sistema','#d4a944','#49350f',false]
].map((g,i)=>({name:g[0],meta:g[1],colors:[g[2],g[3]],favorite:g[4],played:20-i,size:Math.max(.2,4+(i*1.7)),installed:i!==10,platform:i<12?'xbox360':i<14?'arcade':i<16?'kinect':i<18?'homebrew':'apps'}));

function savedNames(key, defaults) {
  try {
    const value = JSON.parse(localStorage.getItem(key));
    if (Array.isArray(value)) return [...new Set(value.filter(name => games.some(game => game.name === name)))];
  } catch { /* Legacy or damaged storage: use the demo's initial collection. */ }
  return defaults;
}
const favorites = savedNames('aurorax-favorites', games.filter(g => g.favorite).map(g => g.name));
games.forEach(game => { game.favorite = favorites.includes(game.name); });
let recentNames = savedNames('aurorax-recents', games.slice(0, 6).map(g => g.name));
const artwork = {'Forza Horizon':'racing', 'Halo 3':'scifi', 'Gears of War 3':'action'};
let detailIndex = -1;
let detailOrigin = null;

function drawShelf(id, entries, empty) {
  const shelf = document.getElementById(id);
  shelf.replaceChildren();
  if (!entries.length) {
    const message = document.createElement('p');
    message.className = 'shelf-empty';
    message.textContent = empty;
    shelf.append(message);
    return;
  }
  for (const game of entries.slice(0, 3)) {
    const button = document.createElement('button');
    button.className = 'shelf-item';
    button.dataset.game = games.indexOf(game);
    const art = document.createElement('span');
    art.className = 'shelf-cover';
    art.style.backgroundImage = 'linear-gradient(145deg,' + game.colors.join(',') + ')';
    const cover = localStorage.getItem('cover-' + game.name);
    if (cover || artwork[game.name]) {
      const img = document.createElement('img');
      img.alt = '';
      img.src = cover || 'art/' + artwork[game.name] + '.jpg';
      img.addEventListener('error', () => img.remove(), {once:true});
      art.append(img);
    }
    const title = document.createElement('strong');
    title.textContent = game.name;
    button.append(art, title);
    shelf.append(button);
  }
}

function drawHome() {
  drawShelf('recentShelf', recentNames.map(name => games.find(game => game.name === name)), 'Os jogos abertos aparecem aqui.');
  drawShelf('favoriteShelf', games.filter(g => g.favorite), 'Use X em um jogo para favoritar.');
}
document.querySelector('.home-shelves').addEventListener('click', event => {
  const card = event.target.closest('[data-game]');
  if (card) openGame(Number(card.dataset.game));
});
let activeFilter = 'all';
let activePlatform = 'all';

const generic = {
  apps:{kicker:'APLICATIVOS',title:'Homebrew & utilitários',text:'Inicie e gerencie ferramentas instaladas.',tabs:['Instalados','Serviços','Atualizações']},
  emulators:{kicker:'EMULADORES',title:'Coleção retrô',text:'ROMs, saves e controles organizados por sistema.',tabs:['Sistemas','ROMs recentes','Configurar']},
  files:{kicker:'ARQUIVOS',title:'Gerenciador de arquivos',text:'Copie e organize HDD, USB e compartilhamentos.',tabs:['HDD1:','USB0:','Rede']},
  system:{kicker:'SISTEMA',title:'Status do console',text:'Monitore hardware, rede, armazenamento e serviços.',tabs:['Visão geral','Armazenamento','Rede','Energia']}
};

const views=document.querySelectorAll('.view');
function show(id){views.forEach(v=>v.classList.remove('active'));const direct=document.querySelector(`#${id}.view`);if(direct)direct.classList.add('active');else{renderModule(id);document.querySelector('#generic').classList.add('active')}window.scrollTo(0,0)}
document.querySelectorAll('[data-view]').forEach(b=>b.addEventListener('click',()=>show(b.dataset.view)));
document.querySelectorAll('[data-library]').forEach(button => button.onclick = () => {
  activePlatform = 'all';
  document.querySelector('#search').value = '';
  show('games');
  syncPlatformTabs();
  document.querySelector(`[data-filter="${button.dataset.library}"]`).click();
});

let currentModule='apps';
function renderModule(id,tab=0){currentModule=id;const d=generic[id];document.querySelector('#genericKicker').textContent=d.kicker;document.querySelector('#genericTitle').textContent=d.title;document.querySelector('#genericText').textContent=d.text;document.querySelector('#moduleNav').innerHTML=d.tabs.map((x,i)=>`<button class="${i===tab?'selected':''}" data-module-tab="${i}">${x}</button>`).join('');document.querySelectorAll('[data-module-tab]').forEach(b=>b.onclick=()=>renderModule(id,+b.dataset.moduleTab));document.querySelector('#moduleActions').innerHTML=moduleActions(id);document.querySelector('#moduleContent').innerHTML=moduleContent(id,tab);wireModuleActions()}
function moduleActions(id){if(id==='files')return '<button data-action="open-device">▱ ABRIR PASTA LOCAL</button><button data-action="new-folder">＋ PASTA</button><button data-action="paste">COLAR</button>';if(id==='system')return '<button data-action="refresh-system">↻ ATUALIZAR</button>';if(id==='emulators')return '<button data-action="scan-roms">↻ ESCANEAR ROMS</button>';return '<button data-action="refresh-apps">↻ ATUALIZAR</button>'}
const appItems=[['Aurora Updater','Atualizações','2.1.0'],['FTP Server','Serviço','Ativo'],['Media Player','Mídia','1.8.2'],['File Manager','Sistema','3.0.1'],['Screenshot Tool','Utilitário','1.2.0'],['DashLaunch','Sistema','3.21']];
const fileItems={0:[['Content','Pasta','—'],['Games','Pasta','—'],['INFINITY 360','Pasta','—'],['launch.ini','Arquivo','4 KB'],['system.log','Arquivo','28 KB']],1:[['Games','Pasta','—'],['ROMs','Pasta','—'],['Covers','Pasta','—']],2:[['MEDIA-SERVER','SMB','Online'],['PC-GAMING','FTP','Online']]};
function moduleContent(id,tab){
 if(id==='apps')return `<div class="data-list">${appItems.filter((_,i)=>tab!==2||i===0).map((x,i)=>`<button data-launch="${x[0]}"><span class="list-icon">${i%2?'⚙':'⌘'}</span><span><b>${x[0]}</b><small>${x[1]}</small></span><i>${x[2]}　▶</i></button>`).join('')}</div>`;
 if(id==='emulators'){const systems=[['Super Nintendo','184 ROMs','◈'],['Mega Drive','126 ROMs','◆'],['Nintendo 64','42 ROMs','●'],['PlayStation','37 ROMs','▣'],['Arcade','312 ROMs','✦'],['Game Boy Advance','98 ROMs','▤']];return `<div class="system-grid">${systems.map(x=>`<button data-system="${x[0]}"><b>${x[2]}</b><strong>${x[0]}</strong><small>${x[1]}</small></button>`).join('')}</div>`}
 if(id==='files')return `<div class="file-toolbar"><span>Dispositivo / ${generic.files.tabs[tab]}</span><span>${tab===0?'102 GB livres':tab===1?'48 GB livres':'2 conexões'}</span></div><div class="data-list files-list">${fileItems[tab].map(x=>`<button data-file="${x[0]}"><span class="list-icon">${x[1]==='Pasta'?'▰':'▱'}</span><span><b>${x[0]}</b><small>${x[1]}</small></span><i>${x[2]}</i></button>`).join('')}</div>`;
 const panels=tab===0?[['CPU','52 °C','Normal'],['GPU','55 °C','Normal'],['MEMÓRIA','61%','Disponível: 198 MB'],['VENTOINHA','65%','Automático']]:tab===1?[['HDD1','218 / 320 GB','68% utilizado'],['USB0','16 / 64 GB','Montado'],['CACHE','1.2 GB','Limpar']]:tab===2?[['ENDEREÇO IP','192.168.1.36','Conectado'],['FTP','Porta 21','Ativo'],['LINK','100 Mbps','Cabo']]:[['TEMPO LIGADO','2h 14m','Sessão atual'],['VENTOINHA','65%','Automático'],['INICIALIZAÇÃO','INFINITY 360','Padrão']];return `<div class="metrics-grid">${panels.map(x=>`<article><small>${x[0]}</small><b>${x[1]}</b><span>${x[2]}</span></article>`).join('')}</div>${tab===3?'<label class="fan-control">Ventoinha manual <input id="fanRange" type="range" min="40" max="100" value="65"><output>65%</output></label>':''}`
}
async function openLocalDirectory(){if(!window.showDirectoryPicker){toast('Seu navegador não oferece acesso local; usando dispositivo virtual');return}try{const handle=await window.showDirectoryPicker({mode:'readwrite'});const rows=[];for await(const [name,entry] of handle.entries())rows.push([name,entry.kind==='directory'?'Pasta':'Arquivo',entry.kind]);document.querySelector('#moduleContent').innerHTML=`<div class="file-toolbar"><span>${handle.name}</span><span>${rows.length} itens</span></div><div class="data-list files-list">${rows.map(x=>`<button data-file="${x[0]}"><span class="list-icon">${x[1]==='Pasta'?'▰':'▱'}</span><span><b>${x[0]}</b><small>${x[1]}</small></span><i>⋮</i></button>`).join('')}</div>`;wireModuleActions()}catch(error){if(error.name!=='AbortError')toast('Não foi possível abrir a pasta')}}
function wireModuleActions(){document.querySelectorAll('[data-launch]').forEach(b=>b.onclick=()=>toast(`Iniciando ${b.dataset.launch}…`));document.querySelectorAll('[data-system]').forEach(b=>b.onclick=()=>toast(`${b.dataset.system}: biblioteca aberta`));document.querySelectorAll('[data-file]').forEach(b=>b.onclick=()=>toast(`${b.dataset.file} selecionado · A abrir · X copiar · Y renomear`));document.querySelectorAll('[data-action]').forEach(b=>b.onclick=()=>{if(b.dataset.action==='open-device')return openLocalDirectory();toast({"new-folder":'Nova pasta criada no dispositivo virtual',paste:'Item colado','refresh-system':'Sensores atualizados','scan-roms':'Varredura concluída: 799 ROMs','refresh-apps':'Aplicativos atualizados'}[b.dataset.action])});const fan=document.querySelector('#fanRange');if(fan)fan.oninput=()=>{fan.nextElementSibling.value=fan.value+'%'}}

function render(filter=activeFilter){
  activeFilter=filter;
  const query=document.querySelector('#search').value.toLowerCase();
  let list=games.filter(g=>g.name.toLowerCase().includes(query) && (activePlatform==='all'||g.platform===activePlatform) && (filter!=='favorite'||g.favorite) && (filter!=='recent'||recentNames.includes(g.name)) && (filter!=='installed'||g.installed));
  const sort=document.querySelector('#sort').value;
  if(filter==='recent')list.sort((a,b)=>recentNames.indexOf(a.name)-recentNames.indexOf(b.name));else if(sort==='played')list.sort((a,b)=>b.played-a.played);else if(sort==='size')list.sort((a,b)=>b.size-a.size);else list.sort((a,b)=>a.name.localeCompare(b.name));
  document.querySelector('#allCount').textContent=games.length;
  document.querySelector('#favoriteCount').textContent=games.filter(g=>g.favorite).length;
  document.querySelector('#gameGrid').innerHTML=list.map(g=>{const cover=localStorage.getItem(`cover-${g.name}`);return `<button class="game-card focusable" data-game="${games.indexOf(g)}"><div class="cover" style="${cover?`background-image:url('${cover}')`:`background:linear-gradient(145deg,${g.colors[0]},${g.colors[1]})`}"><strong>${cover?'':g.name.toUpperCase()}</strong><i class="cover-edit" data-cover="${games.indexOf(g)}" title="Alterar capa">✎</i></div><strong>${g.favorite?'♥ ':''}${g.name}</strong><small>${g.meta} · ${g.size.toFixed(1)} GB ${g.installed?'':'· não instalado'}</small></button>`}).join('') || '<p class="muted">Nenhum título corresponde ao filtro.</p>';
  drawHome();
  document.querySelectorAll('#gameGrid [data-game]').forEach(c=>c.onclick=()=>openGame(+c.dataset.game));
  document.querySelectorAll('[data-cover]').forEach(c=>c.onclick=e=>{e.stopPropagation();openCover(+c.dataset.cover)});
}
function syncPlatformTabs(){document.querySelectorAll('.platform-tabs [data-platform]').forEach(b=>b.classList.toggle('selected',b.dataset.platform===activePlatform))}
document.querySelectorAll('.platform-tabs [data-platform]').forEach(b=>b.onclick=()=>{activePlatform=b.dataset.platform;syncPlatformTabs();render()});
function persist(){localStorage.setItem('aurorax-favorites',JSON.stringify(games.filter(g=>g.favorite).map(g=>g.name)))}
function openGame(index) {
  const game = games[index];
  const modal = document.querySelector('#modal');
  if (!modal.classList.contains('open')) {
    const origin = document.activeElement;
    detailOrigin = {parent: origin.closest('.shelf, #gameGrid')?.id, index};
  }
  detailIndex = index;
  document.querySelector('#modalTitle').textContent = game.name;
  document.querySelector('#modalMeta').textContent = `${game.meta} · ${game.size.toFixed(1)} GB · ${game.installed ? 'Instalado em HDD1' : 'Disponível para instalar'}`;
  document.querySelector('#modalCover').style.background = 'linear-gradient(145deg,' + game.colors.join(',') + ')';
  document.querySelector('#favorite').textContent = game.favorite ? '♥ FAVORITO' : '♡ FAVORITO';
  document.querySelector('#playGame').disabled = !game.installed;
  modal.classList.add('open');
  modal.setAttribute('aria-hidden','false');
  (game.installed ? document.querySelector('#playGame') : document.querySelector('#favorite')).focus();
}
function closeDetails() {
  const modal = document.querySelector('#modal');
  modal.classList.remove('open');
  modal.setAttribute('aria-hidden','true');
  const parent = detailOrigin?.parent && document.getElementById(detailOrigin.parent);
  const focus = parent?.querySelector(`[data-game="${detailIndex}"]`) || parent?.querySelector('button') || document.querySelector('.view.active button');
  focus?.focus();
  detailIndex = -1;
}
document.querySelector('#favorite').onclick = () => {
  const game = games[detailIndex];
  if (!game) return;
  game.favorite = !game.favorite;
  persist();
  render();
  document.querySelector('#favorite').textContent = game.favorite ? '♥ FAVORITO' : '♡ FAVORITO';
};
document.querySelector('#playGame').onclick = () => {
  const game = games[detailIndex];
  if (!game?.installed) return;
  recentNames = [game.name, ...recentNames.filter(name => name !== game.name)];
  localStorage.setItem('aurorax-recents', JSON.stringify(recentNames));
  render();
  closeDetails();
  toast('Execução simulada · ' + game.name);
};
document.querySelector('.modal-close').onclick = closeDetails;
document.querySelector('#search').oninput=()=>render();document.querySelector('#sort').onchange=()=>render();
document.querySelectorAll('[data-filter]').forEach(b=>b.onclick=()=>{document.querySelectorAll('[data-filter]').forEach(x=>x.classList.remove('selected'));b.classList.add('selected');render(b.dataset.filter)});
function toast(text){const t=document.querySelector('#toast');t.textContent=text;t.classList.add('show');setTimeout(()=>t.classList.remove('show'),2200)}
document.addEventListener('click',e=>{const b=e.target.closest('[data-toast]');if(b)toast(b.dataset.toast)});
const guide=document.querySelector('#guide');
function toggleGuide(force){const open=force??!guide.classList.contains('open');guide.classList.toggle('open',open);guide.setAttribute('aria-hidden',String(!open));guide.inert=!open;if(open)document.querySelector('#closeGuide').focus()}
document.querySelector('#closeGuide').onclick=()=>toggleGuide(false);
document.querySelectorAll('#guide [data-view]').forEach(b=>b.addEventListener('click',()=>toggleGuide(false)));
document.querySelector('[data-guide="downloads"]').onclick=()=>toast('2 downloads na fila · 38% concluído');
document.querySelector('[data-guide="network"]').onclick=()=>toast('FTP ativo em 192.168.1.36:21');
document.querySelector('[data-guide="power"]').onclick=()=>toast('Menu de energia protegido por confirmação');
document.querySelector('[data-guide="settings"]').onclick=()=>{toggleGuide(false);document.querySelector('#settingsPanel').classList.add('open')};
document.querySelector('.settings-close').onclick=()=>document.querySelector('#settingsPanel').classList.remove('open');
const settings=JSON.parse(localStorage.getItem('aurorax-settings')||'{}');
if(settings.accent)document.documentElement.style.setProperty('--green',settings.accent);
if(settings.reduceMotion)document.body.classList.add('reduce-motion');
const settingIds=['accent','profileName','reduceMotion','sound','weatherLocation','temperatureUnit','libraryLayout','language','coverServer','autoFtp','autoUpdate','confirmLaunch','parentalControl','parentalPin'];
settingIds.forEach(id=>{const el=document.querySelector(`#${id}`);const key=id==='profileName'?'profile':id;if(settings[key]!==undefined){if(el.type==='checkbox')el.checked=settings[key];else el.value=settings[key]}});
document.querySelector('#accent').value=settings.accent||'#78e819';document.querySelector('#profileName').value=settings.profile||'AX';document.querySelector('#reduceMotion').checked=!!settings.reduceMotion;document.querySelector('#sound').checked=settings.sound!==false;
let selectedTheme=settings.theme||'aurora';
const themes={aurora:['#78e819','#07100e','#101b17'],carbon:['#f0f2f1','#080a0a','#202322'],blades:['#f28c20','#161616','#3a2a1b'],ocean:['#35c8ef','#07131b','#12374a']};
function applyTheme(name){selectedTheme=name;const t=themes[name]||themes.aurora;document.documentElement.style.setProperty('--green',t[0]);document.documentElement.style.setProperty('--theme-bg',t[1]);document.documentElement.style.setProperty('--theme-panel',t[2]);document.querySelectorAll('[data-theme]').forEach(b=>b.classList.toggle('selected',b.dataset.theme===name))}
document.querySelectorAll('[data-theme]').forEach(b=>b.onclick=()=>applyTheme(b.dataset.theme));applyTheme(selectedTheme);if(settings.accent)document.documentElement.style.setProperty('--green',settings.accent);
function readImage(file,done){if(!file)return; if(file.size>1024*1024){toast('Imagem muito grande · máximo 1 MB');return}const reader=new FileReader();reader.onload=()=>done(reader.result);reader.readAsDataURL(file)}
function updateAvatar(data=settings.avatar){const profile=settings.profile||'AX';['avatarInitials','profileInitials'].forEach(id=>document.querySelector(`#${id}`).textContent=profile.slice(0,2).toUpperCase());document.querySelector('#profileTitle').textContent=profile;['avatarImage','profileAvatarImage'].forEach(id=>{const image=document.querySelector(`#${id}`);image.src=data||'';image.hidden=!data})}
document.querySelector('#avatarFile').onchange=e=>readImage(e.target.files[0],data=>{settings.avatar=data;updateAvatar(data)});updateAvatar();
document.querySelector('#avatarButton').onclick=()=>document.querySelector('#profilePanel').classList.add('open');
document.querySelector('.profile-close').onclick=()=>document.querySelector('#profilePanel').classList.remove('open');
document.querySelector('#profilePanel [data-guide="settings"]').onclick=()=>{document.querySelector('#profilePanel').classList.remove('open');document.querySelector('#settingsPanel').classList.add('open')};
let coverGame=-1;
function openCover(index){coverGame=index;const game=games[index];const saved=localStorage.getItem(`cover-${game.name}`);document.querySelector('#coverGameTitle').textContent=game.name;document.querySelector('#coverPreview').style.backgroundImage=saved?`url('${saved}')`:'none';document.querySelector('#coverUrl').value=saved?.startsWith('http')?saved:'';document.querySelector('#coverPanel').classList.add('open')}
document.querySelector('.cover-close').onclick=()=>document.querySelector('#coverPanel').classList.remove('open');
document.querySelector('#coverFile').onchange=e=>readImage(e.target.files[0],data=>{document.querySelector('#coverPreview').dataset.pending=data;document.querySelector('#coverPreview').style.backgroundImage=`url('${data}')`});
document.querySelector('#coverUrl').oninput=e=>{document.querySelector('#coverPreview').dataset.pending=e.target.value;document.querySelector('#coverPreview').style.backgroundImage=e.target.value?`url('${e.target.value}')`:'none'};
document.querySelector('#applyCover').onclick=()=>{if(coverGame<0)return;const value=document.querySelector('#coverPreview').dataset.pending||document.querySelector('#coverUrl').value;if(!value){toast('Selecione um arquivo ou informe uma URL');return}localStorage.setItem(`cover-${games[coverGame].name}`,value);document.querySelector('#coverPanel').classList.remove('open');render();toast('Capa salva no cache local')};
document.querySelector('#removeCover').onclick=()=>{if(coverGame<0)return;localStorage.removeItem(`cover-${games[coverGame].name}`);document.querySelector('#coverPanel').classList.remove('open');render();toast('Capa restaurada')};
document.querySelector('#saveSettings').onclick=()=>{const value={theme:selectedTheme,avatar:settings.avatar||''};settingIds.forEach(id=>{const el=document.querySelector(`#${id}`);value[id==='profileName'?'profile':id]=el.type==='checkbox'?el.checked:el.value});localStorage.setItem('aurorax-settings',JSON.stringify(value));document.documentElement.style.setProperty('--green',value.accent);document.body.classList.toggle('reduce-motion',value.reduceMotion);document.querySelector('#gameGrid').classList.toggle('compact',value.libraryLayout==='compact');Object.assign(settings,value);updateAvatar(value.avatar);updateWeather(value.weatherLocation,value.temperatureUnit);document.querySelector('#settingsPanel').classList.remove('open');toast('Todas as configurações foram salvas')};

const storeItems=[['Tema Blades','Tema gratuito','Uma releitura leve da interface clássica','#6edb49'],['Pacote PT-BR','Tradução gratuita','Textos revisados para a dashboard','#31a7df'],['Capas essenciais','Capas autorizadas','Metadados e artes livres da biblioteca','#ef9034'],['OpenLara 360','Homebrew gratuito','Port comunitário de código aberto','#b365e7'],['Retro Pack','Configuração','Presets para emuladores, sem ROMs','#e74f66'],['Tema Carbon','Tema gratuito','Visual escuro de alto contraste','#748079']];
document.querySelector('#storeGrid').innerHTML=storeItems.map((item,i)=>`<article class="store-item"><div style="--item:${item[3]}">${i%2?'◈':'✦'}</div><p class="eyebrow">${item[1]}</p><h3>${item[0]}</h3><p>${item[2]}</p><progress value="0" max="100"></progress><button data-package="${item[0]}">${localStorage.getItem(`package-${item[0]}`)?'✓ INSTALADO':'⇩ BAIXAR'}</button></article>`).join('');
document.querySelectorAll('[data-package]').forEach(b=>b.onclick=()=>downloadPackage(b));
function downloadPackage(button){if(localStorage.getItem(`package-${button.dataset.package}`)){toast('Pacote já instalado');return}const progress=button.previousElementSibling;button.disabled=true;button.textContent='BAIXANDO 0%';let value=0;const timer=setInterval(()=>{value+=10;progress.value=value;button.textContent=`BAIXANDO ${value}%`;if(value>=100){clearInterval(timer);localStorage.setItem(`package-${button.dataset.package}`,'installed');button.textContent='✓ INSTALADO';button.disabled=false;toast(`${button.dataset.package} verificado e instalado`) }},120)}

const climates={'São Paulo':[24,'☀','Ensolarado'],'Rio de Janeiro':[28,'☀','Céu limpo'],'Curitiba':[17,'☁','Nublado'],'Brasília':[26,'⛅','Parcialmente nublado'],'Lisboa':[20,'🌤','Agradável']};
function updateWeather(city=settings.weatherLocation||'São Paulo',unit=settings.temperatureUnit||'c'){const data=climates[city]||climates['São Paulo'];const temp=unit==='f'?Math.round(data[0]*9/5+32):data[0];const suffix=unit==='f'?'°F':'°';document.querySelector('#weatherCity').textContent=city;document.querySelector('#weatherTitle').textContent=city;document.querySelector('#weatherTemp').textContent=temp+suffix;document.querySelector('#weatherLarge').textContent=temp+suffix;document.querySelector('#weatherIcon').textContent=data[1];document.querySelector('#weatherCondition').textContent=data[2];document.querySelector('#forecast').innerHTML=['HOJE','AMANHÃ','SÁBADO','DOMINGO','SEGUNDA'].map((day,i)=>`<div><small>${day}</small><b>${i%2?'⛅':data[1]}</b><strong>${temp-i}${suffix}</strong></div>`).join('')}
updateWeather();

function updateClock(){const now=new Date();document.querySelector('#clock').textContent=now.toLocaleTimeString('pt-BR',{hour:'2-digit',minute:'2-digit'});document.querySelector('#guideTime').textContent=`Sessão ativa · ${now.toLocaleDateString('pt-BR')}`}
updateClock(); setInterval(updateClock,1000); render();
