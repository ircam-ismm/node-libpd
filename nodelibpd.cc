#include <napi.h>
#include "src/NodePd.h"

// cf. https://github.com/nodejs/abi-stable-node-addon-examples/tree/master/6_object_wrap/node-addon-api
namespace node_lib_pd {

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return NodePd::Init(env, exports);
}

NODE_API_MODULE(nodelibpd, InitAll)

}; // namespace
