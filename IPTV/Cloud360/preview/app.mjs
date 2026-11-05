import {
  MAX_ITEMS, parseM3U, playableUrl, inferType, streamFormat,
  parseXmltv, scheduleFor, normalizeXtream
} from "./catalog.mjs";

const $ = id => document.getElementById(id);
const elements = Object.fromEntries([
  "clock","sourceStatus","hero","heroImport","sectionKicker","sectionTitle","sectionCount",
  "settingsPanel","catalogControls","catalog","paging","moreBtn",
  "search","groupFilter","sortFilter","refreshView","playlistFile","urlForm","playlistUrl",
  "xtreamForm","xtreamServer","xtreamUsername","xtreamPassword","includeVod","epgFile",
  "clearHistory","clearList","viewer","video","videoBlank","closePlayer","watchLogo",
  "watchName","watchGroup","playbackStatus","prevChannel","nextChannel","watchFavorite",
  "fullScreen","epgNow","epgNext","epgProgress","toast","seriesDialog","seriesTitle",
  "seriesEpisodes","seriesClose"
].map(id => [id, $(id)]));
const store = {
  get(key, fallback) { try { return JSON.parse(localStorage.getItem(key) || "") || fallback; } catch { return fallback; } },
  set(key, value) { try { localStorage.setItem(key, JSON.stringify(value)); } catch {} }
};
const state = {
  items: [], filtered: [], tab: "home", page: 1, pageSize: 72,
  favorites: new Set(store.get("cloud360_favorites_v1", [])),
  recent: store.get("cloud360_recent_v1", []), guide: null, active: null,
  hls: null, dash: null, session: null, loading: false, toastTimeout: 0
};
const tabNames = {
  home: ["BIBLIOTECA", "Descubra o catálogo"], live: ["TV AO VIVO", "Todos os canais"],
  movies: ["FILMES", "Biblioteca de filmes"], series: ["SÉRIES", "Temporadas e episódios"],
  favorites: ["SUA LISTA", "Meus favoritos"], recent: ["CONTINUAR", "Assistidos recentemente"],
  guide: ["GRADE DE PROGRAMAÇÃO", "Guia EPG"], settings: ["CONFIGURAÇÕES", "Fontes de conteúdo"]
};
function notify(message) {
  elements.toast.textContent = message; elements.toast.hidden = false;
  clearTimeout(state.toastTimeout);
  state.toastTimeout = setTimeout(() => { elements.toast.hidden = true; }, 6500);
}
function setStatus(message) { elements.playbackStatus.textContent = message; }
function setSourceStatus(message) { elements.sourceStatus.textContent = "● " + message; }
function formatTime(epoch) {
  return new Date(epoch).toLocaleTimeString("pt-BR", { hour: "2-digit", minute: "2-digit" });
}
function currentEpg(item) {
  return scheduleFor(item, state.guide);
}
function logoUrl(item) {
  return item.logo || state.guide?.icons.get(item.tvgId) || "";
}
function addLogo(holder, item, css = "") {
  const url = logoUrl(item);
  const fallback = document.createElement("span");
  fallback.className = "fallback"; fallback.textContent = item.kind === "series" ? "▧" : inferType(item) === "movies" ? "▣" : "▤";
  holder.appendChild(fallback);
  if (url) {
    const img = document.createElement("img");
    img.alt = "Logo de " + item.name; img.loading = "lazy"; img.decoding = "async";
    img.referrerPolicy = "no-referrer";
    img.addEventListener("load", () => { fallback.hidden = true; });
    img.addEventListener("error", () => { img.remove(); fallback.hidden = false; });
    img.src = url; holder.appendChild(img);
  }
}
function remember(item) {
  state.recent = [item.id, ...state.recent.filter(id => id !== item.id)].slice(0, 80);
  store.set("cloud360_recent_v1", state.recent);
}
function favorite(item) {
  if (state.favorites.has(item.id)) state.favorites.delete(item.id);
  else state.favorites.add(item.id);
  store.set("cloud360_favorites_v1", [...state.favorites].slice(0, MAX_ITEMS));
  if (state.active?.id === item.id) elements.watchFavorite.textContent = state.favorites.has(item.id) ? "★" : "☆";
  render();
}
function typeForTab(tab) { return ["live","movies","series"].includes(tab) ? tab : null; }
function filterItems() {
  const query = elements.search.value.trim().toLocaleLowerCase("pt-BR");
  const group = elements.groupFilter.value;
  const type = typeForTab(state.tab);
  const recentRanks = new Map(state.recent.map((id, i) => [id, i]));
  const result = state.items.filter(item => {
    if (type && inferType(item) !== type) return false;
    if (state.tab === "favorites" && !state.favorites.has(item.id)) return false;
    if (state.tab === "recent" && !recentRanks.has(item.id)) return false;
    if (group && item.group !== group) return false;
    if (query && !(item.name + " " + item.group).toLocaleLowerCase("pt-BR").includes(query)) return false;
    return true;
  });
  if (state.tab === "recent") result.sort((a,b) => recentRanks.get(a.id) - recentRanks.get(b.id));
  else if (elements.sortFilter.value === "name") result.sort((a,b) => a.name.localeCompare(b.name, "pt-BR"));
  return result;
}
function refreshGroups() {
  const prior = elements.groupFilter.value;
  const groups = [...new Set(state.items.map(x => x.group))].sort((a,b) => a.localeCompare(b, "pt-BR"));
  elements.groupFilter.replaceChildren(new Option("Todas as categorias", ""));
  for (const group of groups) elements.groupFilter.add(new Option(group, group));
  elements.groupFilter.value = groups.includes(prior) ? prior : "";
}
function setTab(tab) {
  if (!tabNames[tab]) return;
  state.tab = tab; state.page = 1;
  elements.sectionKicker.textContent = tabNames[tab][0];
  elements.sectionTitle.textContent = tabNames[tab][1];
  elements.settingsPanel.hidden = tab !== "settings";
  elements.catalogControls.hidden = tab === "settings";
  elements.paging.hidden = true;
  elements.hero.hidden = tab !== "home";
  document.querySelectorAll("[data-tab]").forEach(button =>
    button.classList.toggle("selected", button.dataset.tab === tab));
  render();
}
function makeChannelCard(item) {
  const card = document.createElement("article"); card.className = "card";
  const kind = inferType(item); card.dataset.kind = kind;
  const artwork = document.createElement("div"); artwork.className = "poster";
  addLogo(artwork, item);
  const body = document.createElement("div"); body.className = "card-body";
  const name = document.createElement("div"); name.className = "card-title"; name.textContent = item.name;
  name.title = item.name;
  const group = document.createElement("div"); group.className = "card-group";
  group.textContent = item.group + " · " + (item.kind === "series" ? "Temporadas" : streamFormat(item.url));
  const schedule = currentEpg(item);
  const now = document.createElement("div"); now.className = "card-epg";
  now.textContent = schedule.now ? "Agora: " + schedule.now.title : " ";
  const actions = document.createElement("div"); actions.className = "card-actions";
  const play = document.createElement("button"); play.type = "button"; play.className = "play";
  play.textContent = item.kind === "series" ? "Ver episódios ▶" : "▶ Assistir";
  play.setAttribute("aria-label", "Abrir player de " + item.name);
  play.addEventListener("click", () => openItem(item));
  const fav = document.createElement("button"); fav.type = "button"; fav.className = "favorite";
  fav.title = state.favorites.has(item.id) ? "Remover favorito" : "Adicionar favorito";
  fav.textContent = state.favorites.has(item.id) ? "★" : "☆";
  fav.addEventListener("click", () => favorite(item));
  actions.append(play, fav);body.append(name,group,now,actions);card.append(artwork,body);
  return card;
}
function render() {
  if (state.tab === "settings") { elements.catalog.replaceChildren(); elements.sectionCount.textContent = "Listas e ajustes"; return; }
  state.filtered = filterItems();
  elements.sectionCount.textContent = state.filtered.length.toLocaleString("pt-BR") + " itens";
  elements.catalog.replaceChildren();
  if (!state.filtered.length) {
    const empty = document.createElement("div"); empty.className = "empty";
    empty.textContent = state.items.length
      ? "Nenhum resultado nesta seção. Tente outra categoria ou busca."
      : "Nenhuma lista carregada. Abra Fontes e ajustes para importar M3U ou Xtream autorizado.";
    elements.catalog.appendChild(empty);elements.paging.hidden = true;return;
  }
  const visible = state.filtered.slice(0, state.page * state.pageSize);
  const fragment = document.createDocumentFragment();
  for (const item of visible) fragment.appendChild(makeChannelCard(item));
  elements.catalog.appendChild(fragment);
  elements.paging.hidden = visible.length >= state.filtered.length;
}
function putCatalog(items, sourceName) {
  state.items = items.slice(0, MAX_ITEMS);
  state.page = 1;state.active = null;stopPlayer();
  state.guide = null; // O XMLTV precisa corresponder à fonte atual.
  elements.groupFilter.value = "";
  elements.search.value = "";
  refreshGroups();setSourceStatus(sourceName + " · " + state.items.length.toLocaleString("pt-BR") + " itens");
  setTab("live");
  notify("Catálogo carregado: " + state.items.length + " entradas.");
}
async function readSmallFile(file, cap, description) {
  if (!file) return null;
  if (file.size > cap) throw Error(description + " excede o limite de " + Math.round(cap/1048576) + " MB nesta prévia.");
  return await file.text();
}
async function importM3U(text, label, baseUrl) {
  const result = parseM3U(text, { baseUrl });
  if (!result.entries.length) throw Error("Nenhum stream HTTP/HTTPS válido encontrado.");
  putCatalog(result.entries, label);
  if (result.truncated) notify("Lista limitada aos primeiros " + MAX_ITEMS + " itens para preservar a memória.");
  else if (result.invalid || result.duplicates) notify("Importados " + result.entries.length + " itens; " + result.invalid + " URLs inválidas e " + result.duplicates + " repetidas.");
}
async function loadUrl() {
  const url = playableUrl(elements.playlistUrl.value);
  if (!url) throw Error("Use uma URL HTTP/HTTPS válida.");
  const response = await fetch(url, { mode: "cors", credentials: "omit", signal: AbortSignal.timeout(20000) });
  if (!response.ok) throw Error("Servidor retornou HTTP " + response.status);
  if (Number(response.headers.get("content-length") || 0) > 20 * 1048576) throw Error("Lista muito grande.");
  const text = await response.text();
  if (text.length > 20 * 1048576) throw Error("Lista acima do limite permitido.");
  await importM3U(text, "M3U remoto", url);
}
async function xtreamRequest(session, action, extra = {}) {
  const url = new URL(session.server + "/player_api.php");
  url.search = new URLSearchParams({ username: session.username, password: session.password, action, ...extra }).toString();
  const response = await fetch(url, { mode:"cors", credentials:"omit", signal: AbortSignal.timeout(20000) });
  if (!response.ok) throw Error(action + ": HTTP " + response.status);
  return await response.json();
}
async function loadXtream() {
  const server = elements.xtreamServer.value.replace(/\/+$/, "");
  if (!playableUrl(server) || !server.toLowerCase().startsWith("https://")) throw Error("Informe um servidor HTTPS válido.");
  const session = { server, username: elements.xtreamUsername.value.trim(), password: elements.xtreamPassword.value };
  if (!session.username || !session.password) throw Error("Informe usuário e senha.");
  const liveGroups = await xtreamRequest(session, "get_live_categories");
  const liveStreams = await xtreamRequest(session, "get_live_streams");
  if (!Array.isArray(liveGroups) || !Array.isArray(liveStreams)) throw Error("Resposta Xtream incompatível.");
  let list = normalizeXtream({ channels: liveStreams, categories: liveGroups, kind: "live", ...session });
  if (elements.includeVod.checked) {
    for (const [kind,categoryAction,streamAction] of [
      ["movie","get_vod_categories","get_vod_streams"],
      ["series","get_series_categories","get_series"]]) {
      try {
        const categories = await xtreamRequest(session, categoryAction);
        const streams = await xtreamRequest(session, streamAction);
        if (Array.isArray(categories) && Array.isArray(streams))
          list.push(...normalizeXtream({ channels: streams, categories, kind, ...session }));
      } catch (err) { notify("Alguns conteúdos não foram carregados: " + String(err.message)); }
    }
  }
  if (!list.length) throw Error("A conta não retornou canais disponíveis.");
  state.session = session; putCatalog(list, "Xtream");
  elements.xtreamPassword.value = ""; // Segredo permanece apenas no objeto da sessão, sem persistência.
}
async function loadEpg(file) {
  const text = await readSmallFile(file, 12*1048576, "XMLTV");
  if (!text) return;
  const guide = parseXmltv(text);
  state.guide = guide; notify("EPG importado: " + guide.total + " programas.");
  refreshWatchEpg();render();
}
function refreshWatchEpg() {
  const item = state.active;
  const data = item ? currentEpg(item) : {now:null,next:null};
  elements.epgNow.textContent = data.now ? "Agora: " + formatTime(data.now.start) + " · " + data.now.title : "Agora: programação indisponível";
  elements.epgNext.textContent = data.next ? "A seguir: " + formatTime(data.next.start) + " · " + data.next.title : "A seguir: —";
  const now = Date.now();
  const percentage = data.now ? Math.max(0, Math.min(100, (now - data.now.start) * 100 / (data.now.stop - data.now.start))) : 0;
  elements.epgProgress.style.width = percentage + "%";
}
function resetEngines() {
  if (state.hls) { state.hls.destroy(); state.hls=null; }
  if (state.dash) { state.dash.reset(); state.dash=null; }
  elements.video.pause();
  elements.video.removeAttribute("src");
  elements.video.load();
}
function stopPlayer() {
  resetEngines();
  elements.videoBlank.hidden = false;
  elements.watchLogo.hidden = true;
  elements.watchName.textContent = "Nenhuma mídia selecionada";
  elements.watchGroup.textContent = "Importe uma lista M3U ou Xtream";
  state.active = null;setStatus("Aguardando conteúdo");refreshWatchEpg();
}
function changeStatus(message) { setStatus(message); }
function handlePlayError() {
  if (!state.active) return;
  const err = elements.video.error;
  if (!err) return;
  const messages = { 1:"Reprodução interrompida.", 2:"Erro de rede ou acesso ao stream.", 3:"Codec ou dados de vídeo incompatíveis.", 4:"Formato ou URL não suportado pelo navegador." };
  changeStatus(messages[err.code] || "O player não conseguiu reproduzir o vídeo.");
}
async function startPlayer(item) {
  resetEngines();state.active = item;
  elements.videoBlank.hidden = true;
  elements.watchName.textContent = item.name;
  elements.watchGroup.textContent = item.group + " · " + streamFormat(item.url);
  elements.watchFavorite.textContent = state.favorites.has(item.id) ? "★" : "☆";
  elements.watchLogo.hidden = true;
  const image = logoUrl(item);
  if (image) { elements.watchLogo.src=image; elements.watchLogo.hidden=false; }
  refreshWatchEpg();
  remember(item);
  changeStatus("Preparando stream...");
  const kind = streamFormat(item.url), video = elements.video;
  if (!playableUrl(item.url)) { changeStatus("URL de mídia inválida."); return; }
  try {
    if (kind === "HLS") {
      if (video.canPlayType("application/vnd.apple.mpegurl")) {
        video.src = item.url;
      } else if (window.Hls?.isSupported()) {
        const hls = new window.Hls({ enableWorker: true, lowLatencyMode: true,
          maxBufferLength: 25, maxMaxBufferLength: 60 });
        state.hls = hls;
        hls.on(window.Hls.Events.ERROR, (_event, data) => {
          if (!data.fatal) return;
          if (data.type === window.Hls.ErrorTypes.MEDIA_ERROR) {
            changeStatus("Recuperando erro de decodificação...");
            hls.recoverMediaError();
          } else {
            changeStatus("Não foi possível reproduzir HLS. Verifique CORS, codecs e autorização.");
            hls.destroy();state.hls=null;
          }
        });
        hls.loadSource(item.url);hls.attachMedia(video);
        await new Promise((resolve,reject) => {
          hls.on(window.Hls.Events.MANIFEST_PARSED, () => resolve());
          hls.on(window.Hls.Events.ERROR, (_ev,data) => { if(data.fatal) reject(new Error("Manifesto HLS indisponível ou incompatível.")); });
        });
      } else { throw Error("HLS não suportado. Verifique a conexão com a biblioteca hls.js."); }
    } else if (kind === "DASH") {
      if (!window.dashjs) throw Error("DASH requer dash.js, indisponível nesta sessão.");
      const dash = window.dashjs.MediaPlayer().create();
      state.dash = dash; dash.initialize(video,item.url,false);
    } else if (kind === "MPEG-TS" || kind === "Contêiner legado") {
      throw Error("Fluxo TS/MKV direto requer um decodificador ou servidor de adaptação. O navegador pode não reproduzir esse formato.");
    } else {
      video.src = item.url;
    }
    const result = video.play();
    if (result) await result;
    changeStatus("Reproduzindo · " + kind);
  } catch(error) {
    changeStatus(error.message || "Falha ao abrir o player.");
  }
}
async function seriesEpisodes(item) {
  if (!state.session) {
    notify("Para séries M3U, selecione um episódio do catálogo. Xtream permite carregar temporadas pela API.");
    return;
  }
  const info = await xtreamRequest(state.session, "get_series_info", { series_id: String(item.seriesId) });
  const seasons = info?.episodes || {};
  elements.seriesTitle.textContent = item.name;
  elements.seriesEpisodes.replaceChildren();
  for (const [season, eps] of Object.entries(seasons)) {
    if (!Array.isArray(eps)) continue;
    const h = document.createElement("h3");h.textContent = "Temporada " + season;
    elements.seriesEpisodes.append(h);
    for (const ep of eps) {
      const id = ep.id; if (!/^\d+$/.test(String(id))) continue;
      const ext = /^(mp4|mkv|ts|avi)$/.test(ep.container_extension || "") ? ep.container_extension : "mp4";
      const url = state.session.server + "/series/" + encodeURIComponent(state.session.username) +
        "/" + encodeURIComponent(state.session.password) + "/" + id + "." + ext;
      const row=document.createElement("div");row.className="episode";
      const label=document.createElement("span");
      label.textContent = (ep.episode_num || "?") + " · " + (ep.title || "Episódio");
      const button=document.createElement("button");button.textContent="▶ Reproduzir";
      button.addEventListener("click",()=> {
        elements.seriesDialog.close();
        startPlayer({ ...item, id:item.id + "/" + id, name:label.textContent, url, kind:"episode" });
      });
      row.append(label,button);elements.seriesEpisodes.append(row);
    }
  }
  if (!elements.seriesEpisodes.children.length) throw Error("Não foram encontrados episódios nesta série.");
  elements.seriesDialog.showModal();
}
async function openItem(item) {
  if (item.kind === "series") {
    try { await seriesEpisodes(item); }
    catch(err) { notify(err.message || "Não foi possível carregar os episódios."); }
    return;
  }
  await startPlayer(item);
  if (window.innerWidth < 1250) elements.viewer.scrollIntoView({ behavior:"smooth", block:"start" });
}
function stepChannel(offset) {
  if (!state.filtered.length) return;
  const index = state.active ? state.filtered.findIndex(x=>x.id===state.active.id) : -1;
  for(let i=1;i<=state.filtered.length;i++) {
    const next=state.filtered[(Math.max(index,0) + offset*i + state.filtered.length * (i+1)) % state.filtered.length];
    if(next && next.kind!=="series") { openItem(next); return; }
  }
}
function focusDirectional(key) {
  const elements = [...document.querySelectorAll("button:not([hidden]),input:not([hidden]),select:not([hidden])")]
    .filter(el=>!el.disabled && el.getClientRects().length);
  if(!elements.length) return;
  const active=document.activeElement;
  const source=active?.getBoundingClientRect?.() || {left:0,top:0,width:0,height:0};
  const from={x:source.left+source.width/2,y:source.top+source.height/2};
  let candidate=null,best=Infinity;
  for(const el of elements){
    if(el===active)continue;
    const r=el.getBoundingClientRect(),to={x:r.left+r.width/2,y:r.top+r.height/2};
    const dx=to.x-from.x,dy=to.y-from.y;
    if(key==="ArrowLeft"&&dx>=-6 || key==="ArrowRight"&&dx<=6 ||
       key==="ArrowUp"&&dy>=-6 || key==="ArrowDown"&&dy<=6)continue;
    const score=(key==="ArrowLeft"||key==="ArrowRight"?Math.abs(dx)+Math.abs(dy)*2:Math.abs(dy)+Math.abs(dx)*2);
    if(score<best){best=score;candidate=el;}
  }
  (candidate || elements[0]).focus();
}
function handleKey(event) {
  if(event.key==="Escape") {
    if(elements.seriesDialog.open)elements.seriesDialog.close();
    else if(state.active)stopPlayer();
    return;
  }
  if(["INPUT","TEXTAREA","SELECT"].includes(document.activeElement?.tagName))return;
  if(event.key==="y" || event.key==="Y") {
    if(state.active)favorite(state.active);
  } else if(event.key==="PageUp"){stepChannel(-1);}
  else if(event.key==="PageDown"){stepChannel(1);}
  else if(["ArrowLeft","ArrowRight","ArrowUp","ArrowDown"].includes(event.key)) {
    event.preventDefault();focusDirectional(event.key);
  }
}
let gamepadPrevious = [],lastGamepadTime=0;
function pollGamepad() {
  const pad = navigator.getGamepads?.().find(x=>x && x.connected);
  if(pad) {
    const down = pad.buttons.map(x=>!!x.pressed);
    if(performance.now()-lastGamepadTime>130) {
      const pressed = n => down[n] && !gamepadPrevious[n];
      if(pressed(0)) document.activeElement?.click?.();
      if(pressed(1)) state.active ? stopPlayer() : setTab("home");
      if(pressed(2) && state.active) elements.fullScreen.click();
      if(pressed(3) && state.active)favorite(state.active);
      if(pressed(4))stepChannel(-1);
      if(pressed(5))stepChannel(1);
      if(pressed(12))focusDirectional("ArrowUp");
      if(pressed(13))focusDirectional("ArrowDown");
      if(pressed(14))focusDirectional("ArrowLeft");
      if(pressed(15))focusDirectional("ArrowRight");
      if(down.some((v,i)=>v&&!gamepadPrevious[i]))lastGamepadTime=performance.now();
    }
    gamepadPrevious=down;
  }
  requestAnimationFrame(pollGamepad);
}
function bind() {
  document.querySelectorAll("[data-tab]").forEach(button=>button.addEventListener("click",()=>setTab(button.dataset.tab)));
  elements.heroImport.addEventListener("click",()=>{setTab("settings");elements.playlistFile.focus();});
  elements.playlistFile.addEventListener("change",async event=>{
    try { const file=event.target.files[0]; const text=await readSmallFile(file,20*1048576,"Playlist");
      if(text)await importM3U(text,file.name); }
    catch(error){notify(error.message);}
    event.target.value="";
  });
  elements.urlForm.addEventListener("submit",async e=>{e.preventDefault();try{await loadUrl();}catch(err){notify("Falha ao carregar lista: "+err.message+" (verifique CORS)");}});
  elements.xtreamForm.addEventListener("submit",async e=>{e.preventDefault();try{await loadXtream();}catch(err){notify("Falha Xtream: "+err.message+" (verifique CORS)");}});
  elements.epgFile.addEventListener("change",async event=>{
    try{await loadEpg(event.target.files[0]);}catch(err){notify(err.message);}
    event.target.value="";
  });
  elements.clearHistory.addEventListener("click",()=>{state.recent=[];store.set("cloud360_recent_v1",[]);render();notify("Histórico removido.");});
  elements.clearList.addEventListener("click",()=>{state.items=[];state.session=null;state.guide=null;stopPlayer();refreshGroups();setTab("settings");setSourceStatus("Sem catálogo");notify("Catálogo removido.");});
  elements.search.addEventListener("input",()=>{state.page=1;render();});
  elements.groupFilter.addEventListener("change",()=>{state.page=1;render();});
  elements.sortFilter.addEventListener("change",()=>{state.page=1;render();});
  elements.refreshView.addEventListener("click",render);
  elements.moreBtn.addEventListener("click",()=>{state.page++;render();});
  elements.closePlayer.addEventListener("click",stopPlayer);
  elements.prevChannel.addEventListener("click",()=>stepChannel(-1));
  elements.nextChannel.addEventListener("click",()=>stepChannel(1));
  elements.watchFavorite.addEventListener("click",()=>{if(state.active) favorite(state.active);});
  elements.fullScreen.addEventListener("click",()=>elements.video.requestFullscreen?.().catch(()=>notify("Tela cheia indisponível.")));
  elements.video.addEventListener("error",handlePlayError);
  elements.video.addEventListener("waiting",()=>{if(state.active)setStatus("Carregando vídeo...");});
  elements.video.addEventListener("playing",()=>{if(state.active)setStatus("Reproduzindo · " + streamFormat(state.active.url));});
  elements.watchLogo.addEventListener("error",()=>{elements.watchLogo.hidden=true;});
  elements.seriesClose.addEventListener("click",()=>elements.seriesDialog.close());
  document.addEventListener("keydown",handleKey);
  setInterval(()=>{elements.clock.textContent=new Date().toLocaleTimeString("pt-BR",{hour:"2-digit",minute:"2-digit"});refreshWatchEpg();},30000);
  elements.clock.textContent=new Date().toLocaleTimeString("pt-BR",{hour:"2-digit",minute:"2-digit"});
  requestAnimationFrame(pollGamepad);
}
bind();
setTab("home");
