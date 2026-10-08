#!/usr/bin/env bash
# Run the pinned library installer with bounded download recovery and verify
# actual build inputs. CI needs project libraries, without a release compiler.
set -euo pipefail

ci_deps_dir=$(cd "${1:?Expected dependency checkout}" && pwd)
case "$(uname -s)" in
  Linux) ci_deps_platform=linux ;;
  Darwin) ci_deps_platform=darwin ;;
  *) echo 'Unix dependency setup requires Linux or macOS' >&2; exit 1 ;;
esac

ci_curl_bin=$(command -v curl)
ci_download_bin=$(mktemp -d)
trap 'rm -rf "$ci_download_bin"' EXIT
printf '#!/usr/bin/env bash\nexec %q --silent --show-error --retry 3 --retry-all-errors --retry-delay 2 --connect-timeout 30 --max-time 180 --retry-max-time 600 "$@"\n' \
  "$ci_curl_bin" > "$ci_download_bin/curl"
chmod +x "$ci_download_bin/curl"

(
  cd "$ci_deps_dir"
  PATH="$ci_download_bin:$PATH" bash scripts/install.sh
)

for ci_deps_file in include/json-c/json.h include/git2.h include/yaml.h \
  lib/libjson-c.a lib/libgit2.a lib/libyaml.a lib/libssh2.a lib/libssl.a \
  lib/libcrypto.a lib/libpcre2-8.a lib/libhttp_parser.a lib/libz.a; do
  if [[ ! -s "$ci_deps_dir/libs/$ci_deps_platform/$ci_deps_file" ]]; then
    echo "Dependency installation is incomplete: $ci_deps_file" >&2
    exit 1
  fi
done
