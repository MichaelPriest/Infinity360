/* The same focus target handles a click, Enter and the controller's A button. */
const homeLibrary = document.getElementById('homeLibrary');
let homeOrigin = homeLibrary;

function inputLayer() {
  return document.querySelector('.modal.open') || document.querySelector('#guide.open') || document.querySelector('.view.active');
}

function focusIn(layer) {
  if (layer?.id === 'home') {
    (homeOrigin.isConnected ? homeOrigin : homeLibrary).focus();
  } else {
    layer?.querySelector('button:not(:disabled), a[href], input:not(:disabled)')?.focus();
  }
}

function back() {
  const layer = inputLayer();
  if (layer.id === 'modal') return closeDetails();
  if (layer.classList.contains('modal')) {
    layer.classList.remove('open');
    layer.setAttribute('aria-hidden','true');
  } else if (layer === guide) {
    toggleGuide(false);
  } else {
    show('home');
  }
  focusIn(inputLayer());
}

function moveFocus(x, y) {
  const layer = inputLayer();
  const active = document.activeElement;
  if (!layer.contains(active)) return focusIn(layer);
  const rect = active.getBoundingClientRect();
  const cx = rect.left + rect.width / 2, cy = rect.top + rect.height / 2;
  let next, best = Infinity;
  for (const candidate of layer.querySelectorAll('button:not(:disabled),a[href],input:not(:disabled),select:not(:disabled)')) {
    if (candidate === active || !candidate.getClientRects().length) continue;
    const target = candidate.getBoundingClientRect();
    const dx = target.left + target.width / 2 - cx, dy = target.top + target.height / 2 - cy;
    const along = x ? x*dx : y*dy;
    if (along <= 4) continue;
    const gap = x ? Math.max(0, target.top-rect.bottom, rect.top-target.bottom)
                  : Math.max(0, target.left-rect.right, rect.left-target.right);
    const score = (gap > 0 ? 1000000 : 0) + gap*12 + along + Math.abs(x ? dy : dx)*.55;
    if (score < best) { next = candidate; best = score; }
  }
  next?.focus();
}

function shortcut(key) {
  if (key === 'back') return back();
  if (key === 'guide') { if (inputLayer().classList.contains('modal')) return; toggleGuide(); if (!guide.classList.contains('open')) focusIn(inputLayer()); return; }
  if (key === 'select') { document.activeElement?.click(); return; }
  if (key === 'favorite') {
    if (inputLayer().id === 'modal') return document.getElementById('favorite').click();
    const card = document.activeElement.closest('[data-game]');
    if (!card) return;
    const index = Number(card.dataset.game), parent = card.parentElement;
    games[index].favorite = !games[index].favorite;
    persist(); render();
    (parent.querySelector(`[data-game="${index}"]`) || parent.querySelector('button') || homeLibrary).focus();
  } else if (key === 'refresh') {
    render();
    if (!inputLayer().contains(document.activeElement)) focusIn(inputLayer());
    toast('Biblioteca atualizada');
  }
}

document.getElementById('guideButton').onclick = () => shortcut('guide');
document.addEventListener('click', event => {
  const trigger = event.target.closest('[data-view],[data-library]');
  if (!trigger) return;
  if (trigger.closest('#home')) homeOrigin = trigger;
  queueMicrotask(() => focusIn(inputLayer()));
}, true);

const arrows = {ArrowLeft:[-1,0], ArrowRight:[1,0], ArrowUp:[0,-1], ArrowDown:[0,1]};
document.addEventListener('keydown', event => {
  if (event.altKey || event.ctrlKey || event.metaKey) return;
  if (event.key === 'Escape') { event.preventDefault(); back(); return; }
  if (event.target.matches('input,textarea,select,[contenteditable=true]')) return;
  const direction = arrows[event.key];
  if (direction) { event.preventDefault(); moveFocus(...direction); return; }
  const key = {g:'guide', b:'back', x:'favorite', y:'refresh', a:'select'}[event.key.toLowerCase()];
  if (key && !event.repeat) { event.preventDefault(); shortcut(key); }
});

let previous = [], padIndex = -1;
function pollPad() {
  if (!document.hidden) {
    let pads;
    try { pads = navigator.getGamepads?.(); } catch { return; }
    const pad = pads && Array.from(pads).find(p => p && p.mapping === 'standard');
    if (pad) {
      const down = Array.from(pad.buttons, button => button.pressed);
      down[12] ||= pad.axes[1] < -.6; down[13] ||= pad.axes[1] > .6;
      down[14] ||= pad.axes[0] < -.6; down[15] ||= pad.axes[0] > .6;
      if (pad.index !== padIndex) previous = down;
      [[0,'select'],[1,'back'],[2,'favorite'],[3,'refresh'],[9,'guide']].forEach(([i,command]) => {
        if (down[i] && !previous[i]) shortcut(command);
      });
      [[12,0,-1],[13,0,1],[14,-1,0],[15,1,0]].forEach(([i,x,y]) => {
        if (down[i] && !previous[i]) moveFocus(x,y);
      });
      previous = down; padIndex = pad.index;
    } else { previous = []; padIndex = -1; }
  }
  requestAnimationFrame(pollPad);
}

const homeIptv = document.getElementById('homeIptv');
homeIptv.addEventListener('click', async event => {
  if (event.ctrlKey || event.metaKey || event.shiftKey || event.altKey) return;
  if (location.protocol === 'file:') {
    event.preventDefault();
    toast('Para abrir o IPTV, execute python -m http.server 8080 na raiz do AuroraX e acesse /Web/.');
    return;
  }
  if (/(?:^|\/)Web\/(?:index\.html)?$/i.test(location.pathname)) return;
  // A server rooted in Web/ may not expose the sibling IPTV preview.
  event.preventDefault();
  try {
    const response = await fetch(homeIptv.href, {method:'HEAD'});
    if (!response.ok && response.status !== 405 && response.status !== 501) throw new Error();
    location.assign(homeIptv.href);
  } catch { toast('Sirva a raiz do AuroraX com python -m http.server 8080 e abra /Web/.'); }
});
if (new URLSearchParams(location.search).get('focus') === 'iptv') homeOrigin = homeIptv;
homeOrigin.focus({preventScroll:true});
requestAnimationFrame(pollPad);
