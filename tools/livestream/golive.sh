#!/usr/bin/env bash
# Render https://hyperactivity.ai in a real Chromium on a virtual display and push
# the screen + the page's audio to one or more RTMP(S) ingests, forever.
#
#   IG_URL=rtmps://edgetee-upload-xxx.xx.fbcdn.net:443/rtmp/  IG_KEY=...  ./golive.sh
#   OUT_EXTRA="rtmp://a.rtmp.youtube.com/live2/KEY|rtmp://live.twitch.tv/app/KEY" ./golive.sh
#   DRYRUN=out.mp4 SECS=10 ./golive.sh      # local test, no network push
#
# Instagram: URL+key come from instagram.com -> Create -> Live video (Live Producer).
# The key is per-broadcast; after starting this, press "Go live" in Live Producer.
set -euo pipefail
PAGE="${PAGE:-https://hyperactivity.ai}"
W="${W:-720}"; H="${H:-1280}"          # IG is 9:16 portrait
FPS="${FPS:-30}"; VBR="${VBR:-3500k}"; DISP="${DISP:-:99}"
CHROME="${CHROME:-$(command -v chromium || command -v chromium-browser || command -v google-chrome || echo /opt/pw-browsers/chromium)}"
[ -d "$CHROME" ] && CHROME="$(find "$CHROME" -type f -name chrome | head -1)"

cleanup(){ kill ${XPID:-} ${CPID:-} ${PPID_:-} 2>/dev/null || true; }
trap cleanup EXIT

Xvfb "$DISP" -screen 0 "${W}x${H}x24" -nolisten tcp & XPID=$!
sleep 1; export DISPLAY="$DISP"

# Audio: a pulse null sink the browser plays into, which ffmpeg records.
AUDIO_IN=(-f lavfi -i "anullsrc=r=48000:cl=stereo")   # silent fallback (ingests require an audio track)
if command -v pulseaudio >/dev/null; then
  pulseaudio --start --exit-idle-time=-1 2>/dev/null || true
  pactl load-module module-null-sink sink_name=live >/dev/null 2>&1 || true
  pactl set-default-sink live
  AUDIO_IN=(-f pulse -i live.monitor)
else
  echo "warn: pulseaudio missing -> streaming silence (apt-get install -y pulseaudio)" >&2
fi

"$CHROME" --no-sandbox --kiosk --start-fullscreen --window-position=0,0 --window-size="$W,$H" \
  --autoplay-policy=no-user-gesture-required --disable-infobars --no-first-run \
  --user-data-dir="$(mktemp -d)" --use-gl=swiftshader "$PAGE" >/dev/null 2>&1 & CPID=$!
sleep "${WARMUP:-8}"

ENC=(-c:v libx264 -preset veryfast -tune zerolatency -pix_fmt yuv420p -r "$FPS" -g $((FPS*2))
     -b:v "$VBR" -maxrate "$VBR" -bufsize "$VBR" -c:a aac -b:a 128k -ar 48000 -ac 2)
IN=(-thread_queue_size 1024 -f x11grab -draw_mouse 0 -video_size "${W}x${H}" -framerate "$FPS" -i "$DISP" -thread_queue_size 1024 "${AUDIO_IN[@]}")

if [ -n "${DRYRUN:-}" ]; then
  exec ffmpeg -hide_banner -loglevel warning -y "${IN[@]}" -t "${SECS:-10}" "${ENC[@]}" "$DRYRUN"
fi

OUTS=()
[ -n "${IG_URL:-}" ] && OUTS+=("[f=flv:onfail=ignore]${IG_URL%/}/${IG_KEY:?IG_KEY needed}")
IFS='|' read -ra EXTRA <<< "${OUT_EXTRA:-}"; for o in "${EXTRA[@]}"; do [ -n "$o" ] && OUTS+=("[f=flv:onfail=ignore]$o"); done
[ ${#OUTS[@]} -gt 0 ] || { echo "no destination: set IG_URL/IG_KEY and/or OUT_EXTRA" >&2; exit 1; }
TEE=$(IFS='|'; echo "${OUTS[*]}")

# Reconnect loop: if an ingest drops, re-push. (IG ends a broadcast at its time cap; a new key is then needed.)
while true; do
  ffmpeg -hide_banner -loglevel warning "${IN[@]}" "${ENC[@]}" -map 0:v -map 1:a -f tee "$TEE" || true
  echo "ffmpeg exited $(date -u +%FT%TZ); restarting in 3s" >&2; sleep 3
done
