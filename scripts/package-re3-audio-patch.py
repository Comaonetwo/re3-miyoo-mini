#!/usr/bin/env python3
"""Small v0.13.1 repair for an installed v0.13; no original game/audio resources."""
import argparse,pathlib,shutil,hashlib,zipfile,struct
p=argparse.ArgumentParser();p.add_argument('kernel',type=pathlib.Path);p.add_argument('helper',type=pathlib.Path);p.add_argument('output',type=pathlib.Path);a=p.parse_args();project=pathlib.Path(__file__).resolve().parents[1]
for f in (a.kernel,a.helper):
 b=f.read_bytes();assert b[:6]==b'\x7fELF\x01\x01' and struct.unpack_from('<H',b,18)[0]==40
root=a.output;folder=root/'Roms/PORTS/Games/GTA3Core13';shortcuts=root/'Roms/PORTS/Shortcuts';folder.mkdir(parents=True,exist_ok=True);shortcuts.mkdir(parents=True,exist_ok=True)
for src,name in [(a.kernel,'re3_cpu'),(a.helper,'re3_audio'),(project/'packaging-sd/re3-audio-launch.sh','audio-launch.sh')]:shutil.copy2(src,folder/name);(folder/name).chmod(0o755)
(folder/'audio-test.sh').write_text('''#!/bin/sh
set -u
cd "$(dirname "$0")" || exit 2
./audio-launch.sh --self-test > audio-test.log 2>&1
result=$?
printf '\\nAudio test exit code: %s\\n' "$result" >> audio-test.log
exit "$result"
''');(folder/'audio-test.sh').chmod(0o755)
(shortcuts/'GTA3 Audio Test.port').write_text('''#!/bin/sh
GameName="GTA3 Audio Test"
GameDir="GTA3Core13"
GameExecutable="audio-test.sh"
GameDataFile="re3_audio"
KillAudioserver=1
PerformanceMode=0
Arguments=""
/mnt/SDCARD/Emu/PORTS/launch_standalone.sh "$GameName" "$GameDir" "$GameExecutable" "$Arguments" "$GameDataFile" "$KillAudioserver" "$PerformanceMode"
''');(shortcuts/'GTA3 Audio Test.port').chmod(0o755)
(root/'README.txt').write_text('''GTA3 v0.13.1 — небольшой звуковой патч для установленного v0.13.

Скопируй Roms в корень SD, объедини папки и замени файлы.
Не удаляй GTA3Core13 перед копированием: здесь нет моделей/музыки!
Файлы game/audio_pcm, игровые сохранения и settings.cfg не затрагиваются.
Обнови список Ports / ~Import ports, затем запусти прежний GTA3 v0.13 Core.

Патч закрепляет пути к аудиопроцессу/музыке независимо от текущей папки
движка, сохраняет IPC дескриптор при exec, ждёт готовность до 8 секунд
и фиксирует причины ошибок exec/mmap/SDL. При наличии Parasyte в Onion
звуковой процесс запускается через его loader и согласованные библиотеки;
иначе используется firmware loader. LD_PRELOAD для аудиопроцесса снимается.
Vendor-библиотеки в патч не включены, используются установленные на SD.

Если музыка снова не слышна, запусти GTA3 Audio Test в Ports.
Он выдаёт один негромкий тон длительностью 2 секунды, затем
вернётся в Ports.
Этот тест не использует файлы GTA и проверяет только звуковой вывод.
При успешном тесте проверить радио в машине и настройки музыки GTA.

Для диагностики нужны журналы из Roms/PORTS/Games/GTA3Core13:
core.log — запуск игры; audio-test.log — отдельный тест звука.
В новой версии они показывают запуск helper, runtime, драйвер SDL,
реальную ошибку загрузки/открытия и наличие звуковых callbacks.
Физический результат этого патча ещё предстоит проверить на Miyoo.

Из прежнего журнала известно только AUDIO unavailable, без причины.
Патч исправляет вероятные проблемы запуска и добавляет доказательства
для следующего шага, если отсутствие звука сохранится. SFX не добавлены.
''')
files=sorted(f for f in root.rglob('*') if f.is_file() and f.name!='SHA256SUMS')
(root/'SHA256SUMS').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.relative_to(root).as_posix()+'\n' for f in files))
with zipfile.ZipFile(str(root)+'.zip','w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 for f in sorted(root.rglob('*')):
  if f.is_file():z.write(f,f.relative_to(root).as_posix())
print('Audio patch:',str(root)+'.zip',pathlib.Path(str(root)+'.zip').stat().st_size,'bytes; no game assets')
