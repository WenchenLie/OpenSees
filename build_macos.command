#!/bin/bash
# 双击此文件即可编译；每行格式为“版本|auto”或“版本|Python 可执行文件绝对路径”。
# auto 会查找该版本，未安装时跳过；填写具体路径时会严格检查，不会跳过错误。
PYTHON_CONFIGURATIONS=(
    "3.11|auto"
    "3.12|auto"
    "3.13|auto"
    "3.14|auto"
    "3.15|auto"
)

set -uo pipefail
REPO_DIR="$(cd "$(dirname "$0")" && pwd)" || exit 1
cd "$REPO_DIR" || exit 1
# Finder 启动 Terminal 时不一定继承 Homebrew 的 PATH。
if [[ "$(uname -m)" == "arm64" ]]; then
    export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
else
    export PATH="/usr/local/bin:/opt/homebrew/bin:$PATH"
fi
LOG_DIR="$REPO_DIR/build/macos-$(uname -m)/logs"
mkdir -p "$LOG_DIR" || exit 1
LOG_FILE="$LOG_DIR/$(date '+%Y%m%d-%H%M%S')-$$.log"
ARGS=()
for entry in "${PYTHON_CONFIGURATIONS[@]}"; do
    ARGS+=(--python "$entry")
done
printf 'OpenSees macOS 多版本编译\n日志：%s\n\n' "$LOG_FILE"
if command -v python3 >/dev/null 2>&1; then
    PYTHONUNBUFFERED=1 python3 "$REPO_DIR/multi_python_build_macos.py" "${ARGS[@]}" "$@" 2>&1 | tee "$LOG_FILE"
    result=${PIPESTATUS[0]}
else
    printf '未找到 Python 3，请先安装 Python。\n' | tee "$LOG_FILE"
    result=1
fi
if [[ "$result" -eq 0 ]]; then
    printf '\n完成。日志：%s\n' "$LOG_FILE"
else
    printf '\n编译失败（退出码 %s）。请查看日志：%s\n' "$result" "$LOG_FILE"
fi
# 双击启动时保留窗口；带参数运行或设置 OPENSEES_NO_PAUSE=1 时直接退出。
if [[ $# -eq 0 && -t 0 && "${OPENSEES_NO_PAUSE:-0}" != "1" ]]; then
    read -r -p '按回车键关闭窗口……' unused
fi
exit "$result"
