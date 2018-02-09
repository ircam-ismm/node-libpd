const path = require('path');
const fs = require('fs');

// debug
const SegfaultHandler = require('segfault-handler');
SegfaultHandler.registerHandler("crash.log");

const patchesPath = path.join(process.cwd(), 'test', 'pd');

/**
 * import pd instance
 */
const pd = require('../');

/**
 * list methods
 */
{
  console.log('************************************************');
  console.log('* API *');
  for (let i in pd)
    console.log(`- ${i}`);
  console.log('************************************************');
}

/**
 * Start worker thread, launch pd and portaudio
 * @todo - fix the race condition between the js and the worker thread
 */
const initialized = pd.init({
  numInputChannels: 0,
  numOutputChannels: 1,
  sampleRate: 48000,
  ticks: 1,
});

console.log('');
console.log('[pd initialized]', initialized);
console.log('');

/**
 * open / close patch
 */
// console.log('');
// console.log('>>>> open invalid patches');
// console.log('');

// const doNotExistsPatch = pd.openPatch('do-not-exists.pd', patchesPath);
// console.log('patch 1 not-opened: ', doNotExistsPatch);

// console.log('');
// console.log('>>>> close invalid patches');
// console.log('');

// try {
//   pd.closePatch('');
// } catch(err) { console.error(err) }

// try {
//   pd.closePatch({});
// } catch(err) { console.error(err) }

// const dummyPatch = {
//   isValid: 1,
//   filename: 'dummy.pd',
//   path: '/a/b/c',
//   $0: 42,
// };

// pd.closePatch(dummyPatch);
// console.log(dummyPatch);

// console.log('');
// console.log('>>>>> open / close patches');
// console.log('');

// const patch1 = pd.openPatch('open-close.pd', patchesPath);
// console.log('patch 1 opened: ', patch1);

// setTimeout(() => {
//   const patch2 = pd.openPatch('open-close.pd', patchesPath);
//   console.log('patch 2 opened: ', patch2);

//   setTimeout(() => {
//     pd.closePatch(patch1);
//     pd.closePatch(patch2);
//     // close several time to test if crashes
//     pd.closePatch(patch2);
//     pd.closePatch(patch2);

//     console.log('patch 1 closed: ', patch1);
//     console.log('patch 2 closed: ', patch2);
//   }, 1000);
// }, 1000);


/**
 * currentTime
 */
// function logCurrentTime() {
//   const currentTime = pd.currentTime;
//   console.log('currentTime:', currentTime);

//   setTimeout(logCurrentTime, 100);
// };

// logCurrentTime();


/**
 * Audio Input / Output
 */
// console.log('');
// console.log('>>>>> audio in / out');
// console.log('');

// const audioIOPatch = pd.openPatch('audio-input.pd', patchesPath);

// setInterval(() => {
//   pd.send('tone');
// }, 2000);

/**
 * Subscribe / Unsubscribe
 */
// console.log('');
// console.log('>>>>> subscribe / unsubscribe');
// console.log('');

// const subscriptionPatch = pd.openPatch('subscribe-unsubscribe.pd', patchesPath);
// console.log('subscribing "subscription-test" channel');

// var callback = function() { console.log('bang'); };
// pd.subscribe('subscription-test', callback);

// setTimeout(() => {
//   console.log('unsubscribing "subscription-test" channel');
//   pd.unsubscribe('subscription-test', callback);
// }, 2000);

/**
 * Receive
 */
// console.log('');
// console.log('>>>>> receive types');
// console.log('');

// const sendPatch = pd.openPatch('send-msg.pd', patchesPath);

// pd.subscribe("bangFromPd", function() { console.log('bang !'); });
// pd.subscribe("floatFromPd", function(num) { console.log('num', num) });
// pd.subscribe("symbolFromPd", function(symbol) { console.log('symbol', symbol); });
// pd.subscribe("listFromPd", function(list) { console.log('list', list); });

/**
 * Send
 */
// console.log('');
// console.log('>>>>> send / receive');
// console.log('');

// const patch = pd.openPatch('receive-msg.pd', patchesPath);
// const $0 = patch.$0;

// pd.subscribe(`${$0}-log-bang`, function() { console.log('bang'); });
// pd.subscribe(`${$0}-log-float`, function(val) { console.log(val); });
// pd.subscribe(`${$0}-log-symbol`, function(val) { console.log(val); });
// pd.subscribe(`${$0}-log-list`, function(val) { console.log(val); });

// const now = pd.currentTime;

// pd.send(`${$0}-bang`, true);
// pd.send(`${$0}-float`, 42, now + 1);
// pd.send(`${$0}-symbol`, 'mySymbol', now + 2);
// pd.send(`${$0}-list`, ['test', 21, 'niap', true /* ignored */, 0.3], now + 3);

/**
 * Sine
 */
// console.log('');
// console.log('>>>>> sine');
// console.log('');

// const patch = pd.openPatch('sine.pd', patchesPath);

// setTimeout(() => {
//   pd.closePatch(patch);
// }, 5 * 1000);


/**
 * Poly
 */
// console.log('');
// console.log('>>>>> poly like');
// console.log('');

// for (let i = 0; i < 3; i++) {
//   setTimeout(() => {
//     const patch = pd.openPatch('poly-like.pd', patchesPath);
//     console.log(patch);
//     const baseFreq = 200 * (i + 1);
//     let index = 0;

//     const intervalId = setInterval(() => {
//       pd.send(patch.$0 + '-freq', (index + 1) * baseFreq);
//       pd.send(patch.$0 + '-trigger');

//       index += 1;

//       if (index >= 16)
//         clearInterval(intervalId);
//     }, 300);
//   }, 200 * (i + 1));
// }

/**
 * Poly scheduled
 */
console.log('');
console.log('>>>>> poly scheduled');
console.log('');

const patches = [];
for (let i = 0; i < 3; i++)
  patches[i] = pd.openPatch('poly-like.pd', patchesPath);

const startDelay = 0.2; // in sec
const noteDelay = 0.3; // in sec
const now = pd.currentTime;

for (let i = 0; i < 3; i++) {
  const startOffset = now + (i * startDelay);
  const baseFreq = 200 * (i + 1);

  for (let j = 0; j < 16; j++) {
    const triggerTime = startOffset + j * noteDelay;
    pd.send(patches[i].$0 + '-freq', baseFreq * (j + 1), triggerTime);
    pd.send(patches[i].$0 + '-trigger', true, triggerTime);
  }
}

