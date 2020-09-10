// const path = require('path');
// const fs = require('fs');
// const assert = require('assert');


// // debug
// const SegfaultHandler = require('segfault-handler');
// SegfaultHandler.registerHandler("crash.log");

// const patchesPath = path.join(process.cwd(), 'test', 'pd');

// /**
//  * import pd instance
//  */
// const pd = require('../');

// /**
//  * list methods
//  */
// {
//   console.log('************************************************');
//   console.log('* API *');
//   for (let i in pd)
//     console.log(`- ${i}`);
//   console.log('************************************************');
// }

// /**
//  * Start worker thread, launch pd and portaudio
//  * @todo - fix the race condition between the js and the worker thread
//  */
// const initialized = pd.listDevices();
