const path = require('path');
const fs = require('fs');
const assert = require('chai').assert;


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

describe('node-libpd', () => {
  it('pd.init(config)', () => {
    // start worker thread, launch pd and portaudio
    // @todo - fix the race condition between the js and the worker thread
    const initialized = pd.init({
      numInputChannels: 1,
      numOutputChannels: 1,
      sampleRate: 48000,
      ticks: 1,
    });

    assert.isTrue(initialized);
  });

  let openClosePatch1 = null;
  let openClosePatch2 = null;

  it('pd.openPatch(filename, path)', function() {
    // this.timeout(2100);
    assert.throws(() => {
      pd.openPatch();
    }, Error, `Invalid Arguments: pd.openPatch(filename, path)`);

    openClosePatch1 = pd.openPatch('open-close.pd', patchesPath);
    assert.isObject(openClosePatch1);
    assert.isTrue(openClosePatch1.isValid);
    assert.equal(openClosePatch1.filename, 'open-close.pd');
    assert.equal(openClosePatch1.path, patchesPath);
    assert.isFinite(openClosePatch1.$0);

    const inexistingPatch = pd.openPatch('abcd.pd', patchesPath);
    assert.isFalse(inexistingPatch.isValid);
    assert.equal(inexistingPatch.filename, '');
    assert.equal(inexistingPatch.path, '');
    assert.equal(inexistingPatch.$0, 0);
  });

  it('pd.openPatch(pathname)', function() {
    const filename = path.join(patchesPath, 'open-close.pd');
    openClosePatch2 = pd.openPatch(filename);

    assert.isObject(openClosePatch2);
    assert.isTrue(openClosePatch2.isValid);
    assert.equal(openClosePatch2.filename, 'open-close.pd');
    assert.equal(openClosePatch2.path, patchesPath);
    assert.isFinite(openClosePatch2.$0);

    const inexistingPatch = pd.openPatch('test/abcd.pd');
    assert.isFalse(inexistingPatch.isValid);
    assert.equal(inexistingPatch.filename, '');
    assert.equal(inexistingPatch.path, '');
    assert.equal(inexistingPatch.$0, 0);
  });

  it('pd.closePatch(patch)', function() {
    assert.throws(() => {
      pd.closePatch('something');
    }, Error, "Invalid Arguments: pd.closePatch(patch)");

    assert.throws(() => {
      pd.closePatch({});
    }, Error, "Invalid Arguments: pd.closePatch(patch)");

    closedPatch1 = pd.closePatch(openClosePatch1);
    assert.strictEqual(closedPatch1, openClosePatch1);
    assert.isFalse(closedPatch1.isValid);
    assert.equal(closedPatch1.filename, 'open-close.pd');
    assert.equal(closedPatch1.path, patchesPath);
    assert.equal(closedPatch1.$0, 0);

    const closedPatch2 = pd.closePatch(openClosePatch2);
    assert.strictEqual(closedPatch2, openClosePatch2); // same ref
    assert.isFalse(openClosePatch2.isValid);
    assert.equal(openClosePatch2.filename, 'open-close.pd');
    assert.equal(openClosePatch2.path, patchesPath);
    assert.equal(openClosePatch2.$0, 0);
  });

  it('pd.currentTime', function(done) {
    this.timeout(2000);
    let lastTime = pd.currentTime;
    let counter = 0;

    const intervalId = setInterval(() => {
      const currentTime = pd.currentTime;
      console.log('> currentTime:', currentTime);
      // we tolerate 0.02 errors because of timeout inaccuracy and bufferSize
      // @note - thats a lot...
      const marginOfError = 0.02;
      assert.approximately(lastTime + 0.1, currentTime, marginOfError);
      lastTime = currentTime;

      counter += 1;

      if (counter >= 10) {
        clearInterval(intervalId);
        setTimeout(() => done(), 500);
      }
    }, 100);
  });

  it('pd.send(channel, msg)', function(done) {
    const patch = pd.openPatch('echo-msg.pd', patchesPath);
    console.log(`send:`);
    console.log(`> true`);
    console.log(`> 42`);
    console.log(`> "mySymBol"`);
    console.log(`> ['test', 21, 'niap', true, 0.3]`);
    console.log(`should echo in pd style (bang float symbol, list):`);
    pd.send(`${patch.$0}-bang`, true);
    pd.send(`${patch.$0}-float`, 42);
    pd.send(`${patch.$0}-symbol`, 'mySymbol');
    pd.send(`${patch.$0}-list`, ['test', 21, 'niap', true /* ignored */, 0.3]);

    // wait a bit because send is async
    setTimeout(() => done(), 100);
  });

  it('pd.send(channel, msg) - charge test - double free fixed in #1171345', function(done) {
    this.timeout(15000);
    console.log('send lots of message every 10ms during 10s');
    const patch = pd.openPatch('send-charge-test.pd', patchesPath);
    let int = 0;

    const intervalId = setInterval(() => {
      for (let i = 0; i < 100; i++) {
        pd.send(`${patch.$0}-list`, [int++]);
      }
    }, 10);

    setTimeout(() => {
      clearInterval(intervalId);
      console.log(`sent ${int / 100} list message composed of 100 float values`);

      done();
    }, 10000);
  });

  it('pd.send(channel, msg, time) - schedule message', function(done) {
    this.timeout(10000);
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
    setTimeout(() => {
      done();
    }, 5000);
  });

  it('pd.receive(channel, callback) - types', function(done) {
    this.timeout(10000);
    const patch = pd.openPatch('send-random-msg.pd', patchesPath);
    const types = [false, false, false, false];

    function checkDone() {
      let isDone = types.reduce((acc, value) => acc && value, true);
      if (isDone) {
        pd.unsubscribe(`bangFromPd`);
        pd.unsubscribe(`floatFromPd`);
        pd.unsubscribe(`symbolFromPd`);
        pd.unsubscribe(`listFromPd`);
        done();
      }
    }

    pd.subscribe(`bangFromPd`, function() {
      console.log('bang');
      assert.ok('bang');
      types[0] = true;
      checkDone();
    });

    pd.subscribe(`floatFromPd`, function(value) {
      console.log(value);
      assert.isNumber(value);
      types[1] = true;
      checkDone();
    });

    pd.subscribe(`symbolFromPd`, function(value) {
      console.log(value);
      assert.isString(value);
      types[2] = true;
      checkDone();
    });

    pd.subscribe(`listFromPd`, function(value) {
      console.log(value);
      assert.isArray(value);
      types[3] = true;
      checkDone();
    });
  });


  it('pd.receive(channel, callback) - message order', function(done) {
    const patch = pd.openPatch('echo-msg.pd', patchesPath);
    let messageCounter = 0;
  //   console.log(`send:`);
  //   console.log(`> true`);
  //   console.log(`> 42`);
  //   console.log(`> "mySymBol"`);
  //   console.log(`> ['test', 21, 'niap', true, 0.3]`);
  //   console.log(`should echo in pd style (bang float symbol, list):`);

    var callback = function() { console.log('unsubbed'); };
    pd.subscribe(`${patch.$0}-bang-echo`, function() {
      assert.equal(messageCounter, 0);
      messageCounter += 1;
    });
    pd.subscribe(`${patch.$0}-float-echo`, function(num) {
      assert.equal(num, 42);
      assert.equal(messageCounter, 1);
      messageCounter += 1;
    });
    pd.subscribe(`${patch.$0}-symbol-echo`, function(symbol) {
      assert.equal(symbol, 'mySymbol');
      assert.equal(messageCounter, 2);
      messageCounter += 1;
    });
    pd.subscribe(`${patch.$0}-list-echo`, function(list) {
      console.log('[warning] boolean are ignored in lists');
      assert.notDeepEqual(list, ['test', 21, 'niap', true /* ignored */, 12]);
      assert.deepEqual(list, ['test', 21, 'niap', 12]);
      assert.equal(messageCounter, 3);
      messageCounter += 1;
    });

    pd.send(`${patch.$0}-bang`, true);
    pd.send(`${patch.$0}-float`, 42);
    pd.send(`${patch.$0}-symbol`, 'mySymbol');
    pd.send(`${patch.$0}-list`, ['test', 21, 'niap', true /* ignored */, 12]);

    setTimeout(() => {
      if (messageCounter === 0) {
        assert.fail('no messages received');
      } else if (messageCounter > 4) {
        assert.fail('received messages after unsubscribe');
      }
      done();
    }, 1000);
  });


  it('pd.send() | pd.receive() - echo charge test - check if we loose message', function(done) {
    this.timeout(15000);
    console.log('send 10 echo message every 10ms during 10s');

    const patch = pd.openPatch('echo-msg-nolog.pd', patchesPath);
    let send = 0;
    let expected = 0;

    pd.subscribe(`${patch.$0}-float-echo`, value => {
      expected += 1;
      assert.equal(expected, value);
    });

    const intervalId = setInterval(() => {
      for (let i = 0; i < 10; i++) {
        send += 1;
        pd.send(`${patch.$0}-float`, send);
      }

      if (send >= 10000) {
        clearInterval(intervalId);
        setTimeout(() => {
          console.log(`send ${send} messages`);
          console.log(`received ${expected} messages in right order`);
          done();
        }, 100);
      }
    }, 10);
  });


////
//// Not updated !!!!
////

//   it('sine', () => {
//     const patch = pd.openPatch('sine.pd', patchesPath);

//     setTimeout(() => {
//       pd.closePatch(patch);
//       done();
//     }, 5 * 1000);
//   });

//   it('poly-like', () => {
//     for (let i = 0; i < 3; i++) {
//       setTimeout(() => {
//         const patch = pd.openPatch('poly-like.pd', patchesPath);
//         console.log(patch);
//         const baseFreq = 200 * (i + 1);
//         let index = 0;

//         const intervalId = setInterval(() => {
//           pd.send(patch.$0 + '-freq', (index + 1) * baseFreq);
//           pd.send(patch.$0 + '-trigger');

//           index += 1;

//           if (index >= 16)
//             clearInterval(intervalId);
//         }, 300);
//       }, 200 * (i + 1));
//     }
//     setTimeout(() => {
//       done();
//     }, 5000);
//   });

//   it('writing arrays', () => {
//     const patch = pd.openPatch('array-test.pd', patchesPath);
//     const size = pd.arraySize('my-array');

//     const source = new Float32Array(20);

//     for (let i = 0; i < 20; i++) {
//       source[i] = i / 20;
//     }

//     const written = pd.writeArray('my-array', source);
//     for (let i = 0; i < 20; i++) {
//       pd.send('index', i);
//     }
//     setTimeout(() => {
//       done();
//     }, 5000);
//   });

  //   it('subscribe/unsubscribe', () => {
//     const subscriptionPatch = pd.openPatch('subscribe-unsubscribe.pd', patchesPath);
//     console.log('subscribing "subscription-test" channel');

//     var callback = function() { console.log('bang'); };
//     pd.subscribe('subscription-test', callback);

//     setTimeout(() => {
//       console.log('unsubscribing "subscription-test" channel');
//       pd.unsubscribe('subscription-test', callback);
//       done();
//     }, 2000);
//   });

//   it('unsubscribe without subscribe', () => {
//     const subscriptionPatch = pd.openPatch('subscribe-unsubscribe.pd', patchesPath);

//     const callback = function() { console.log('bang'); };
//     pd.unsubscribe('subscription-test', callback);
//   });

  // it('[audio] Input / Output fun', function(done) {
  //   this.timeout(10000);
  //   const audioIOPatch = pd.openPatch('audio-input.pd', patchesPath);

  //   const intervalId = setInterval(() => pd.send('tone'), 100);

  //   setTimeout(() => {
  //     clearInterval(intervalId);
  //     done();
  //   }, 5000);
  // });


  it('pd.destroy() - warning ! any interaction with `pd` after that will throw a segfault error', () => {
    pd.destroy();
    assert(true);
    console.log('the script should terminate now');
  });
});
