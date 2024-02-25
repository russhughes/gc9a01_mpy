#!/bin/sh

SIZE=20
../../utils/write_font_converter.py \
    -s "万事起头难。熟能生巧。冰冻三尺，非一日之寒。三个臭皮匠，胜过诸葛亮。今日事，今日毕。师父领进门，修行在个人。一口吃不成胖子。欲速则不达。百闻不如一见。不入虎穴，焉得虎子。" \
	NotoSansSC-Regular.otf ${SIZE} >proverbs_${SIZE}.py

mpy-cross proverbs_${SIZE}.py
mpremote cp proverbs_${SIZE}.mpy :
