#!/usr/bin/env python3
"""Create a PRIVATE SD test package from the owner's converted scene.
Do not publish the output: it contains derived proprietary game assets.
The independent source archive contains original project code only.
"""
import argparse, pathlib, struct, shutil, zipfile, hashlib
p=argparse.ArgumentParser();p.add_argument('binary',type=pathlib.Path);p.add_argument('scene',type=pathlib.Path);p.add_argument('output',type=pathlib.Path)
p.add_argument('--driving-assets',type=pathlib.Path)
p.add_argument('--standalone',action='store_true',help='Independent v0.10 game folder and shortcut directly in Ports')
a=p.parse_args();project=pathlib.Path(__file__).resolve().parents[1]
b=a.binary.read_bytes()
if b[:6]!=b'\x7fELF\x01\x01' or struct.unpack_from('<H',b,18)[0]!=40:raise ValueError('requires ELF32 little-endian ARM binary')
offset=struct.unpack_from('<I',b,28)[0];stride,count=struct.unpack_from('<HH',b,42)
for i in range(count):
 if struct.unpack_from('<I',b,offset+i*stride)[0] in (2,3):raise ValueError('binary must be static (no DYNAMIC/INTERP)')
magic=a.scene.read_bytes()[:8]
if magic not in (b'M3SCENE1',b'M3SCENE2',b'M3SCENE3'):raise ValueError('invalid scene magic')
textured=magic in (b'M3SCENE2',b'M3SCENE3')
game_dir='GTA3Textures' if textured else 'GTA3Chinatown'
game_name='GTA3 Chinatown Textures' if textured else 'GTA3 Chinatown'
if a.driving_assets:
 game_dir='GTA3Drive';game_name='GTA3 Driving'
if a.standalone:
 if not a.driving_assets:raise ValueError('--standalone requires --driving-assets')
 game_dir='GTA3Drive10';game_name='GTA3 v0.10'
root=a.output;game=root/'Roms/PORTS/Games'/game_dir;shortcuts=root/'Roms/PORTS/Shortcuts/Experimental'
if a.standalone:shortcuts=root/'Roms/PORTS/Shortcuts'
(game/'assets').mkdir(parents=True,exist_ok=True);shortcuts.mkdir(parents=True,exist_ok=True)
shutil.copy2(a.binary,game/'gta3_soft_demo');(game/'gta3_soft_demo').chmod(0o755)
shutil.copy2(a.scene,game/'assets/chinatown.m3s')
manifest=a.scene.with_suffix('.json')
if manifest.exists():shutil.copy2(manifest,game/'assets/chinatown.json')
if a.driving_assets:
 for name,magic in [('kuruma.m3s',b'M3SCENE2'),('chinatown.m3c',b'M3COL001')]:
  source=a.driving_assets/name
  if source.read_bytes()[:8] not in ((b'M3SCENE2',b'M3SCENE3') if name=='kuruma.m3s' else (magic,)):raise ValueError('invalid driving asset: '+name)
  shutil.copy2(source,game/'assets'/name)
 shutil.copy2(a.driving_assets/'driving.json',game/'assets/driving.json')
shutil.copytree(project/'licenses',game/'licenses',dirs_exist_ok=True)
(game/'run.sh').write_text('''#!/bin/sh
set -u
cd "$(dirname "$0")" || exit 1
chmod a+x ./gta3_soft_demo
./gta3_soft_demo --scene assets/chinatown.m3s > demo.log 2>&1
result=$?
if [ "$result" -eq 42 ]; then
    # Matches Onion write_mainui_state(MAIN_MENU): reset only temporary UI state.
    if [ -d /mnt/SDCARD/.tmp_update ]; then
        rm -f /tmp/state.json
    fi
    printf '\\nExit requested: Onion main menu\\n' >> demo.log
    result=0
fi
printf '\\nLauncher exit code: %s\\n' "$result" >> demo.log
if [ "$result" -ne 0 ] && command -v infoPanel >/dev/null 2>&1; then
    infoPanel --title "GTA3 Chinatown" --message "Test could not start. Details in:\\nRoms/PORTS/Games/GTA3Chinatown/demo.log"
fi
exit "$result"
''');(game/'run.sh').chmod(0o755)
(shortcuts/(game_name+'.port')).write_text('''#!/bin/sh
GameName="GTA3 Chinatown"
GameDir="GTA3Chinatown"
GameExecutable="run.sh"
GameDataFile="chinatown.m3s"
KillAudioserver=0
PerformanceMode=0
Arguments=""
/mnt/SDCARD/Emu/PORTS/launch_standalone.sh "$GameName" "$GameDir" "$GameExecutable" "$Arguments" "$GameDataFile" "$KillAudioserver" "$PerformanceMode"
''');(shortcuts/(game_name+'.port')).chmod(0o755)
# Onion launch_standalone.sh uses find -iname (basename), maxdepth 2.
if not any(f.is_file() and f.name.lower()=='chinatown.m3s' and len(f.relative_to(game).parts)<=2 for f in game.rglob('*')):
 raise ValueError('scene does not satisfy Onion basename/depth presence check')
(game/'_required_files.txt').write_text('Prepared from your own PC GTA III CD: assets/chinatown.m3s\n')
(root/'README.txt').write_text('''GTA III / Miyoo Mini + Plus: Чайнатаун, эксперимент v0.3

Пакет создан локально из твоего купленного оригинального CD.
Он содержит преобразованные модели GTA III. Только для твоей личной проверки;
не публикуй этот архив. Исходники движка в отдельном архиве не содержат ассетов.

1. Распакуй архив на компьютере.
2. Скопируй папку Roms из архива в корень SD-карты, объединив папки.
   Предыдущее демо останется: у этой версии отдельная папка GTA3Chinatown.
3. На OnionOS запусти Ports → Experimental → GTA3 Chinatown.
   Если пункт не появился, обнови список игр; Ports должен быть включён.

Управление:
  D-Pad вверх/вниз: движение; влево/вправо: поворот камеры.
  A: вернуть камеру на стартовую улицу, дальность 80 м.
  B: переключить дальность 80 / 160 м (D80 / D160 сверху).
  L: опустить камеру; R: поднять. Коллизий пока нет.
  Start: пауза. X: перевернуть экран. Menu + Select: главный экран OnionOS. Select: выход в список портов.

Это просмотр настоящего участка карты: 398 объектов, 44 268 треугольников.
Программная графика 320×240, цвета вершин/материалов, простое освещение.
Текстур, персонажа, движения машин, физики, звука и миссий пока нет.
Это следующий прототип рендерера, ещё не запускающийся игровой re3.

Версии v0.2 и v0.3 уже проверены тобой на консоли; у v0.3 хороший FPS.
Эта новая сборка проверена на компьютере, но её FPS на Miyoo ещё неизвестен.
Начни с D80. Если медленно — сообщи FPS; если пустой экран — нажми A.
Журнал: Roms/PORTS/Games/GTA3Chinatown/demo.log.
После теста полезны FPS при D80/D160, читаемость сцены и этот журнал.
''')
if textured:
 for path in (game/'run.sh',shortcuts/(game_name+'.port')):
  content=path.read_text().replace('GTA3Chinatown',game_dir).replace('GTA3 Chinatown',game_name)
  path.write_text(content)
 text=(root/'README.txt').read_text().replace('эксперимент v0.3','текстуры v0.4.1').replace('GTA3Chinatown',game_dir).replace('GTA3 Chinatown',game_name)
 text=text.replace('  Start: пауза.', '  Y: переключить текстуры / плоские цвета (T / F сверху).\n  Start: пауза.')
 text=text.replace('Текстур, персонажа, движения машин, физики, звука и миссий пока нет.',
                   'Добавлены текстуры до 64×64, перспективная коррекция, отсечение прозрачных пикселей.\nПерсонажа, движения машин, физики, звука и миссий пока нет.')
 text=text.replace('FPS при D80/D160,', 'FPS при D80/D160 с T/F,')
 (root/'README.txt').write_text(text)
if a.driving_assets:
 run=game/'run.sh'
 run.write_text(run.read_text().replace('--scene assets/chinatown.m3s','--drive assets/chinatown.m3s assets/kuruma.m3s assets/chinatown.m3c'))
 (root/'README.txt').write_text('''GTA III / Miyoo Mini + Plus — Driving v0.10

Готовый личный тестовый пакет из твоего купленного GTA III CD.
Содержит преобразованные игровые ресурсы: не публикуй архив.

Скопируй папку Roms в корень SD-карты с объединением папок.
Запусти Ports → Experimental → GTA3 Driving.
Папка GTA3Drive обновляет заезд v0.5; демо карты остаются отдельно.
Если пункт не появился, обнови список игр в OnionOS.

Управление:
  Вверх — газ; вниз — торможение, затем задний ход.
  Влево/вправо — руль. L — тормоз.
  R — переключить езду / свободную камеру. В свободной камере L — подняться.
  A — начать заезд заново, вернуть машину и камеру на старт, дальность 80 м.
  B — дальность 80 / 160 м. Y — текстуры / плоские цвета.
  Start — пауза. X — перевернуть экран.
  Menu + Select — главный экран OnionOS; Select отдельно ничего не делает.
  Menu отдельно (отпустить) — день / ночь.
  Удерживая Menu, нажми Y — следующий цвет машины (пять цветов).
  Удерживая Menu, нажми B — полное / упрощённое затенение (RW HI / RW LO).
  Эти сочетания не переключают текстуры/дальность и не меняют день/ночь.
  Если FPS недостаточен, начни с D80, RW LO и дня.
  В подсказках M обозначает Menu, S — Select, Q — качество.

Добавлены оригинальная Kuruma, камера позади машины и столкновения
с дорогой/зданиями по COL-файлам выбранного участка. Машина наклоняется
по высоте под четырьмя колёсами; у края карты останавливается.
Пять жёлтых контрольных точек: проезжай по порядку до финиша.
Таймер начинается при движении, останавливается на финише/паузе/
в свободной камере. После финиша можно ездить дальше; A — новый заезд.
Мини-карта показывает машину, направление и точки; жёлтая — следующая.
Передние колёса поворачиваются, все колёса вращаются от движения.
Это наш прототип, ещё не игровой цикл re3: нет персонажа, штатных миссий,
трафика, звука, повреждений и штатной физики GTA III.

Графика программная 320×240, текстуры до 64×64. Начни с D80 и T.
v0.10 содержит новые данные моделей M3SCENE3: исходные цвета вершин,
нормали, коэффициенты освещения и непрерывную прозрачность материала.
Восстановлены 24 треугольника стёкол Kuruma, ранее отброшенные конвертером.
Город и машина освещаются настоящими ambient/directional lights librw.
Прозрачные поверхности идут отдельным проходом с сортировкой.
Освобождены промежуточные модели/текстуры после создания объектов librw.
Ускорен общий растеризатор: интерполяция атрибутов по строкам;
на компьютере кадр с новыми ресурсами стал примерно на 10% быстрее
того же кадра с предыдущим растеризатором. FPS Miyoo пока неизвестен.
19 проверок Release и ASan/UBSan прошли на компьютере.
На консоли FPS и память новой версии ещё нужно проверить.

Проверь стёкла/покраску, день/ночь, RW HI/LO, пять точек, финиш, A,
паузу, свободную камеру и Menu + Select. Настройки пока не сохраняются.
На мини-карте верх — север (+Z сцены). Это упрощённая схема COL, не карта GTA.
Полезны FPS в движении при D80/D160 и T/F, снимок проблемного места.
Журнал: Roms/PORTS/Games/GTA3Drive/demo.log.
В журнале ожидается rw_unsupported=0, есть rw_atomics, rw_shaded,
night/quality/paint и peak_rss_kib/current_rss_kib.
''')
 (game/'_required_files.txt').write_text('Prepared privately from your owned PC GTA III: assets/chinatown.m3s, assets/kuruma.m3s, assets/chinatown.m3c\n')
if a.standalone:
 text=(root/'README.txt').read_text().replace('GTA3Drive','GTA3Drive10').replace('Ports → Experimental → GTA3 Driving','Ports → GTA3 v0.10')
 text=text.replace('Папка GTA3Drive10 обновляет заезд v0.5; демо карты остаются отдельно.',
  'Полный пакет: программа, город, Kuruma и столкновения включены.\nПапка GTA3Drive10 отдельная; предыдущие версии не требуются.')
 text=text.replace('Если пункт не появился, обнови список игр в OnionOS.',
  'После копирования открой Games, нажми SELECT → Refresh all roms → A.\nЗатем в Ports запусти ~Import ports и открой GTA3 v0.10.\nЯрлык лежит прямо в Ports, вне папки Experimental.\nЕсли Ports отсутствует: Apps → Package Manager → Verified → Ports Collection.')
 text+='\nВажно: копируй Roms именно в корень карты. Не создавай Roms/Roms.\nПроверка на компьютере: должны быть файлы\nRoms/PORTS/Shortcuts/GTA3 v0.10.port\nRoms/PORTS/Games/GTA3Drive10/assets/chinatown.m3s\nRoms/PORTS/Games/GTA3Drive10/assets/kuruma.m3s\nRoms/PORTS/Games/GTA3Drive10/assets/chinatown.m3c\n'
 (root/'README.txt').write_text(text)
checks=[]
for f in sorted(root.rglob('*')):
 if f.is_file() and f.name!='SHA256SUMS':checks.append(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.relative_to(root).as_posix())
(root/'SHA256SUMS').write_text('\n'.join(checks)+'\n')
archive=pathlib.Path(str(root)+'.zip')
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for f in sorted(root.rglob('*')):
  if f.is_file():z.write(f,f.relative_to(root))
print(archive,archive.stat().st_size,'bytes; PRIVATE owner assets included')
