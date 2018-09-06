const nodelibpd = require('bindings')('nodelibpd');

/**
 * @namespace pd
 *
 */
/**
 * @method pd.send
 * @todo - note on messsages order
 *
 * @param {String} channel - name of the corresponding receive in pd patch
 * @param {Any} value - payload of the message, the corresponding mapping is
 *  made with pd types:
 *  - Number -> float
 *  - String -> symbol
 *  - Array  -> list (all value that neither Number nor String are ignored)
 *  - else   -> bang
 * @param {Number} [time=undefined] - audio time at which the message should be
 *  sent. If undefined or < currentTime, is sent as fast as possible. Messages
 *  are processed at pd control rate.
 */

// singleton
let _pd = new nodelibpd.NodePd();
let _listeners = {};

// set message listener
_pd._setMessageCallback(function(channel, value) {
  const listeners = _listeners[channel];

  // as a message can still arrive from pd after an `unsubscribe` call because
  // of the inter-processs queue, we must check if listeners still exists
  if (listeners && listeners.length > 0)
    listeners.forEach(function(listener) { listener(value); });
});

// don't allow somebody to use that (look like we can't delete it...  )
_pd._setMessageCallback = null;

/**
 * public `subscribe` / `unsubscribe` API
 * overrides the API defined in the addon to implement callback management
 */
const nativeSubscribe = _pd.subscribe.bind(_pd);
const nativeUnsubscribe = _pd.unsubscribe.bind(_pd);

_pd.subscribe = function(channel, callback) {
  if (!_listeners[channel]) {
    _listeners[channel] = [];
    nativeSubscribe(channel);
  }

  _listeners[channel].push(callback);
}

_pd.unsubscribe = function(channel, callback) {
  const listeners = _listeners[channel];
  const index = listeners.indexOf(callback);

  if (index !== -1) {
    listeners.splice(index, 1);

    if (listeners.length === 0) {
      nativeUnsubscribe(channel);
      delete _listeners[channel];
    }
  }
}

module.exports = _pd;
