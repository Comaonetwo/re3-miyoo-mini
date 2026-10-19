#!/usr/bin/env python3
"""PRIVATE local console package. Original owned assets; no upstream gamefiles or exe."""
import argparse,pathlib,shutil,zipfile,hashlib,struct,json
p=argparse.ArgumentParser();p.add_argument('binary',type=pathlib.Path);p.add_argument('owned_game',type=pathlib.Path);p.add_argument('output',type=pathlib.Path);p.add_argument('--audio',required=True,type=pathlib.Path);p.add_argument('--audio-helper',required=True,type=pathlib.Path);a=p.parse_args();project=pathlib.Path(__file__).resolve().parents[1]
b=a.binary.read_bytes()
if b[:6]!=b'\x7fELF\x01\x01' or struct.unpack_from('<H',b,18)[0]!=40:raise ValueError('requires ARM32 ELF')
off=struct.unpack_from('<I',b,28)[0];stride,count=struct.unpack_from('<HH',b,42)
for i in range(count):
 if struct.unpack_from('<I',b,off+i*stride)[0] in (2,3):raise ValueError('requires static executable')
root=a.output;folder=root/'Roms/PORTS/Games/GTA3Core29';game=folder/'game';shortcuts=root/'Roms/PORTS/Shortcuts';game.mkdir(parents=True,exist_ok=True);shortcuts.mkdir(parents=True,exist_ok=True)
folders={d.name.lower():d for d in a.owned_game.iterdir() if d.is_dir()}
for name in ['models','data','text','anim','txd']:
 if name not in folders:raise ValueError('missing own '+name)
 for source in folders[name].rglob('*'):
  if not source.is_file():continue
  rel=source.relative_to(folders[name]);normalized='/'.join(part.lower() for part in rel.parts)
  if normalized in ('txd.img','txd.dir','caps.dat'):continue # generated GPU cache is incompatible
  dest=game/name/normalized;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dest)
for required in ['models/gta3.img','models/gta3.dir','models/coll/peds.col','data/gta3.dat','data/main.scm','anim/ped.ifp','text/american.gxt']:
 if not (game/required).is_file():raise ValueError('missing '+required)
audio_binary=a.audio_helper.read_bytes()
if audio_binary[:6]!=b'\x7fELF\x01\x01' or struct.unpack_from('<H',audio_binary,18)[0]!=40:raise ValueError('audio helper must be ARM32 ELF')
manifest=json.loads((a.audio/'manifest.json').read_text());tracks=manifest['tracks']
if manifest['rate']!=22050 or len(tracks)!=196 or any(t.get('missing') for t in tracks):raise ValueError('complete owned 196-track PCM conversion required')
for t in tracks:
 f=a.audio/f"{t['id']:03}.pcm"
 if hashlib.sha256(f.read_bytes()).hexdigest()!=t['pcm_sha256']:raise ValueError('audio checksum '+str(f))
shutil.copy2(project/'packaging-sd/re3-audio-launch.sh',folder/'audio-launch.sh');(folder/'audio-launch.sh').chmod(0o755);
shutil.copytree(a.audio,game/'audio_pcm',dirs_exist_ok=True)
owned_sfx={f.name.lower():f for f in folders.get('audio',a.owned_game/'audio').iterdir() if f.is_file()}
(game/'audio').mkdir(exist_ok=True)
for name in ['sfx.raw','sfx.sdt']:
 if name not in owned_sfx:raise ValueError('missing own '+name)
 shutil.copy2(owned_sfx[name],game/'audio'/name)
shutil.copy2(a.audio_helper,folder/'re3_audio');(folder/'re3_audio').chmod(0o755)
shutil.copy2(a.binary,folder/'re3_cpu');(folder/'re3_cpu').chmod(0o755);shutil.copytree(project/'licenses',folder/'licenses',dirs_exist_ok=True)
(folder/'run.sh').write_text('''#!/bin/sh
set -u
export LD_LIBRARY_PATH="/mnt/SDCARD/miyoo/lib:/usr/lib:/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$(dirname "$0")" || exit 1
gta_cpu_governor_path=/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
gta_prev_governor=""
if [ -r "$gta_cpu_governor_path" ] && [ -w "$gta_cpu_governor_path" ]; then
 gta_prev_governor=$(cat "$gta_cpu_governor_path" 2>/dev/null)
 case "$gta_prev_governor" in performance|ondemand|conservative|powersave|userspace|interactive) ;; *) gta_prev_governor="" ;; esac
 if [ -n "$gta_prev_governor" ]; then printf 'performance\\n' > "$gta_cpu_governor_path" 2>/dev/null || true; fi
fi
trap 'if [ -n "$gta_prev_governor" ]; then printf "%s\\n" "$gta_prev_governor" > "$gta_cpu_governor_path" 2>/dev/null || true; fi' 0
./re3_cpu game --world 0 --console > core.log 2>&1
result=$?
if [ "$result" -eq 42 ]; then
 if [ -d /mnt/SDCARD/.tmp_update ]; then rm -f /tmp/state.json; fi
 result=0
fi
printf '\\nLauncher exit code: %s\\n' "$result" >> core.log
exit "$result"
''');(folder/'run.sh').chmod(0o755)
(shortcuts/'GTA3 v0.29 Core.port').write_text('''#!/bin/sh
GameName="GTA3 v0.29 Core"
GameDir="GTA3Core29"
GameExecutable="run.sh"
GameDataFile="re3_cpu"
KillAudioserver=1
PerformanceMode=0
Arguments=""
/mnt/SDCARD/Emu/PORTS/launch_standalone.sh "$GameName" "$GameDir" "$GameExecutable" "$Arguments" "$GameDataFile" "$KillAudioserver" "$PerformanceMode"
''');(shortcuts/'GTA3 v0.29 Core.port').chmod(0o755)
(folder/'audio-test.sh').write_text('#!/bin/sh\ncd "$(dirname "$0")" || exit 2\n./audio-launch.sh --self-test > audio-test.log 2>&1\nresult=$?\nprintf "Audio test exit code: %s\\n" "$result" >> audio-test.log\nexit "$result"\n');(folder/'audio-test.sh').chmod(0o755)
(shortcuts/'GTA3 v0.29 Audio Test.port').write_text('#!/bin/sh\nGameName="GTA3 v0.29 Audio Test"\nGameDir="GTA3Core29"\nGameExecutable="audio-test.sh"\nGameDataFile="re3_audio"\nKillAudioserver=1\nPerformanceMode=0\nArguments=""\n/mnt/SDCARD/Emu/PORTS/launch_standalone.sh "$GameName" "$GameDir" "$GameExecutable" "$Arguments" "$GameDataFile" "$KillAudioserver" "$PerformanceMode"\n');(shortcuts/'GTA3 v0.29 Audio Test.port').chmod(0o755)
(root/'README.txt').write_text('''GTA3 v0.29 Core — гудок L2/R2 и оптимизация меню/HUD, со штатным стартовым меню.

Это личный пакет из твоих купленных дисков GTA III. Не публикуй архив.
Скопируй Roms в корень SD с объединением папок, обнови список игр,
Ports → ~Import ports → GTA3 v0.29 Core. v0.12 остаётся отдельно.

Defaults: мир 160x120, HUD/штатное меню 640x480, дальность 60 м,
FLAT, текстуры ON, тени OFF. В Menu можно выбрать мир 80x60,
160x120, 240x180, 320x240 и HUD RESOLUTION 240x180, 320x240 или 640x480.
Шрифты сохраняют исходную детализацию; штатное меню больше не рисуется
в низком разрешении мира. Реальный FPS и нагрузку HUD 640 нужно сравнить
на консоли. Расчёт цветов одноцветных 2D спрайтов упрощён.

Потоковый звук: радио в машине, окружение, музыкальные события и дорожки
сцен подключены через MusicManager re3. На ПК проверено воспроизведение
в отдельном процессе; вывод звука v0.14 уже подтверждён владельцем на Miyoo.
При ошибке аудио игра продолжит работать без звука; сохраняй core.log.
Добавлен SFX-микшер до 32 каналов: шаги, транспорт, выстрелы, интерфейс
и голоса из собственного PC RAW/SDT банка; частота, громкость и циклы
управляются re3. Банк отображается в память, без полного копирования
77 МБ в RAM. Вывод моно, 3D/reverb не реализованы. Работа SFX подтверждена на Miyoo в v0.14.
Дорожки заранее преобразованы в моно PCM 22050 Hz: на консоли нет
затрат на декодирование MP3/ADPCM и загрузку целой станции в память.
Используется установленная на SD /miyoo/lib/libSDL-1.2.so.0 и системные
библиотеки прошивки. Vendor библиотеки в архив не включены. В библиотечные пути добавлен /config/lib: прошлый журнал
указывал на отсутствующую libmi_common.so. Audioserver
Onion останавливается стандартным механизмом Ports во время запуска.

Menu — наше меню настроек/пауза; D-pad — выбор/изменение; A — выбрать;
B/Start — назад. Настройки сохраняются при закрытии/выходе в settings.cfg.
Start в игре — штатное меню GTA. L2/R2 в машине — гудок. Menu+Select — выход в Onion;
Menu+X — переворот. Одиночный Select не выходит.
D-pad — движение/руль; A — бег/газ; X — прыжок/тормоз; B — огонь;
Y — вход/выход из машины; L/R — камера пешком, L1/R1 в машине.
В стандартной схеме управления L в машине переключает радио.
Select+L/R — L1/R1 пешком; Select+X/Y — смена оружия.

Проверь читаемость штатного меню Start, мир 240x180, HUD 640/320,
затем сядь в машину и послушай радио. В пешем режиме ожидаются фоновые
дорожки окружения, а не постоянная музыка. Громкость музыки задаётся
в штатных настройках GTA и аппаратным регулятором консоли.
Если звука нет, запусти GTA3 v0.29 Audio Test (негромкий тон 2 секунды)
и сохрани audio-test.log. Затем приложи Roms/PORTS/Games/GTA3Core29/core.log:
строки AUDIO ready/START или SDL failure помогут найти причину.

Сохранения прошлых самостоятельных сборок автоматически не перенесены: для переноса отдельно
скопируй game/userfiles между папками билдов. Исправлены цвет заливки шрифтов (2D depth equality), маска радара
(экранная глубина), сглаживание 2D. Изображения меню сохранены.
Добавлен Skin/HAnim CPU проход: кости и освещение голов катсцен.
Текст/логотип меню 640x480; фон выбирается 640/320/240 через MENU BACKGROUND.
B включает штатный автоаим (пистолет/Uzi/дробовик/AK/M16), B+L/R меняет цель.
AUTO AIM ON/OFF в нашем меню; Select+R — ручное прицеливание/прицел.
Дальность 40/60/80/100 м. Загрузка сейвов и перезапуск мира работают;
новые сейвы в game/userfiles/GTA3sfN.b. Старые game/N.b копируются туда
без удаления оригиналов и без замены существующих слотов.
Фон mainmenu24 сохранён в исходных 512x512.
Оптимизированы фильтрация 2D и точные таблицы оттенков мира.
WORK содержит sim/world/frame/visibility/effects/blur/ui_ms; FRAME — skins.
Старый билд можно оставить
для сравнения. Для сброса настроек нового удали GTA3Core29/settings.cfg.
HUD 240 — отдельный прозрачный слой, мир вне него не уменьшается.
Вывод кадра выполняется при обновлении, RGB32 путь ускорен.
Штатный Quit game теперь возвращает в Onion. PCM предварительно
буферизуется; лог AUDIO CLOCK показывает rate и underruns.
Обычный governor performance выбирается только на время запуска и
восстанавливается. Разгон не применяется. Полные миссии/долгие сеансы,
прирост FPS и головы катсцен нового v0.29 требуют проверки на консоли.
''')
files=sorted(f for f in root.rglob('*') if f.is_file() and f.name!='SHA256SUMS')
(root/'SHA256SUMS').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.relative_to(root).as_posix()+'\n' for f in files))
with zipfile.ZipFile(str(root)+'.zip','w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for f in sorted(root.rglob('*')):
  if f.is_file():z.write(f,f.relative_to(root).as_posix())
print('PRIVATE standalone:',str(root)+'.zip','binary SHA256:',hashlib.sha256(b).hexdigest(),'binary bytes:',len(b))
