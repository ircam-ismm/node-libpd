var nodelibpd = require('bindings')('nodelibpd');

// singleton
var _pd = new nodelibpd.NodePd();
var _listeners = {};

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
