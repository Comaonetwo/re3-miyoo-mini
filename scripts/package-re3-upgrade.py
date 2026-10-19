#!/usr/bin/env python3
"""Asset-free Miyoo executable update with a Ports shortcut."""
import argparse,pathlib,shutil,hashlib,zipfile,struct,re
p=argparse.ArgumentParser()
p.add_argument('kernel',type=pathlib.Path);p.add_argument('helper',type=pathlib.Path);p.add_argument('output',type=pathlib.Path)
p.add_argument('--installed-folder',choices=['GTA3Core14','GTA3Core13'],default='GTA3Core14')
a=p.parse_args();project=pathlib.Path(__file__).resolve().parents[1]
version=re.search(r'GTA3_PORT_VERSION\s+"([0-9.]+)"',(project/'src/port_version.h').read_text()).group(1)
for f in (a.kernel,a.helper):
 b=f.read_bytes()
 if b[:6]!=b'\x7fELF\x01\x01' or struct.unpack_from('<H',b,18)[0]!=40:raise ValueError('requires ARM32 ELF: '+str(f))
b=a.kernel.read_bytes();off=struct.unpack_from('<I',b,28)[0];stride,count=struct.unpack_from('<HH',b,42)
if any(struct.unpack_from('<I',b,off+i*stride)[0]in(2,3)for i in range(count)):raise ValueError('kernel must be static')
root=a.output
if root.exists()or pathlib.Path(str(root)+'.zip').exists():raise ValueError('output exists; preserve old releases')
folder=root/'Roms/PORTS/Games'/a.installed_folder;folder.mkdir(parents=True)
for src,name in [(a.kernel,'re3_cpu'),(a.helper,'re3_audio'),(project/'packaging-sd/re3-audio-launch.sh','audio-launch.sh'),(project/'packaging-sd/re3-core-launch.sh','run.sh')]:
 shutil.copy2(src,folder/name);(folder/name).chmod(0o755)
shutil.copytree(project/'licenses',folder/'licenses')
shortcuts=root/'Roms/PORTS/Shortcuts';shortcuts.mkdir(parents=True)
shortcut=shortcuts/('GTA3 v'+version+'.port')
shortcut.write_text('''#!/bin/sh
GameName="GTA3 vVERSION"
GameDir="FOLDER"
GameExecutable="run.sh"
GameDataFile="re3_cpu"
KillAudioserver=1
PerformanceMode=0
Arguments=""
/mnt/SDCARD/Emu/PORTS/launch_standalone.sh "$GameName" "$GameDir" "$GameExecutable" "$Arguments" "$GameDataFile" "$KillAudioserver" "$PerformanceMode"
'''.replace('VERSION',version).replace('FOLDER',a.installed_folder));shortcut.chmod(0o755)
(root/'README.txt').write_text('''GTA III vVERSION для Miyoo Mini / OnionOS
Обновление существующей установки v0.58, с ярлыком GTA3 vVERSION.

УСТАНОВКА
Объедини Roms из архива с Roms в корне карты, согласись на замену файлов.
Папку игры целиком не удаляй. Не создавай вложенную папку Roms/Roms.
Данные игры, сейвы и settings.cfg в архив не входят и не перезаписываются.
Ярлык: Ports -> GTA3 vVERSION. Старый ярлык тоже запускает обновлённый файл;
если новый не появился сразу, обнови список игр в OnionOS.
Нужна твоя существующая установка собственных файлов GTA III в
Roms/PORTS/Games/FOLDER/game. Игровых ресурсов, музыки и сейвов в ZIP нет.

ИЗМЕНЕНИЯ
Ускорение рендера мира, ранний пропуск гарантированно невидимых поверхностей,
точный NEON расчёт цвета для части HIGH, меньше служебных попиксельных счётчиков.
Дополнительно ускорен HUD240x180 в CUSTOM. MEDIUM/HIGH сохраняют HUD320x240.
AUTO: после быстрого отката неудачного повышения разрешения повторная попытка
отложена на8 секунд активной игры. Обычные18/21FPS и приоритеты сохранены.
Звуковая логика0.58 сохранена, звукового RAM-кэша нет.
На компьютере отрисовка быстрее примерно на20% MEDIUM /22% HIGH относительно0.58;
это НЕ измеренный прирост FPS Mini. На консоли новую версию ещё надо проверить.

КОРОТКИЙ ТЕСТ
1. MEDIUM + DYNAMIC WORLD AUTO: поездка3-5мин, при возможности дождь.
2. HIGH + AUTO: похожий маршрут3-5мин. Оцени плавность, картинку и переключения.
3. Для сравнения скорости отдельно CUSTOM: мир240x180, HUD320x240,
   дальность100, Dynamic World OFF. Сравни ту же сцену с0.58.
4. Проверь обычные выстрелы/столкновения и выход MENU+SELECT.
   Полный повтор гаражей/сейвов не требуется без нового симптома.
Сохрани FOLDER/core.log до следующего запуска: журнал перезаписывается.
В меню и загрузчике должна отображаться VERSION.
AUTO может потратить ускорение на повышение качества, сохраняя близкий FPS.
'''.replace('VERSION',version).replace('FOLDER',a.installed_folder),encoding='utf-8')
files=sorted(f for f in root.rglob('*')if f.is_file())
(root/'SHA256SUMS').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.relative_to(root).as_posix()+'\n'for f in files))
with zipfile.ZipFile(str(root)+'.zip','w',zipfile.ZIP_DEFLATED,compresslevel=9)as z:
 for f in sorted(root.rglob('*')):
  if f.is_file():z.write(f,f.relative_to(root).as_posix())
print('Asset-free update:',str(root)+'.zip')
