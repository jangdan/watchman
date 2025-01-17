/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <folly/String.h>
#include <folly/memory/Malloc.h>
#include "watchman/watchman_cmd.h"

using namespace watchman;

#if defined(FOLLY_USE_JEMALLOC) && !FOLLY_SANITIZE

/** This command is present to manually trigger a
 * heap profile dump when jemalloc is in use.
 * Since there is a complicated relationship with our build system,
 * it is only included in the folly enabled portions of watchman.
 */
static void cmd_debug_prof_dump(
    struct watchman_client* client,
    const json_ref&) {
  if (!folly::usingJEMalloc()) {
    throw std::runtime_error("jemalloc is not in use");
  }

  auto result = mallctl("prof.dump", nullptr, nullptr, nullptr, 0);
  auto resp = make_response();
  resp.set(
      "prof.dump",
      w_string_to_json(
          folly::to<std::string>(
              "mallctl prof.dump returned: ", folly::errnoStr(result))
              .c_str()));
  send_and_dispose_response(client, std::move(resp));
}
W_CMD_REG("debug-prof-dump", cmd_debug_prof_dump, CMD_DAEMON, NULL)
#endif
