@echo off
setlocal enabledelayedexpansion

rem ������Ŀ�ļ�·���������ʵ������޸�
set "projectFile=C:\Users\zheng\Desktop\��Ŀ\Split\Split\Split.vcxproj"
rem ����Ҫ���ҵ� AdditionalOptions ��ǩ��ʼ�ͽ�������
set "searchStart=<AdditionalOptions>"
set "searchEnd=</AdditionalOptions>"
rem ����̶��� AdditionalOptions ����
set "fixedOptions=-mllvm -bcf -mllvm -bcf_prob="
set "subOptions=-mllvm -sub -mllvm -sub_loop="
set "splitOptions=-mllvm -split_num="
set "aesSeedOptions=-mllvm -aesSeed="

rem ��������� bcf_prob ֵ����Χ 1 �� 30
set /a "bcfProb=!random! %% 15 + 5"
rem ��������� sub_loop ֵ����Χ 0 �� 3
set /a "subLoop=!random! %% 1 + 1"
rem ��������� split_num ֵ����Χ 0 �� 3
set /a "splitNum=!random! %% 1 + 1"

rem ���� aesSeed �ĳ���
set "aesSeedLength=32"
rem ��������ַ���
set "characters=0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"

rem ��������� aesSeed ֵ
set "aesSeed="
for /l %%i in (1,1,%aesSeedLength%) do (
    set /a "randomIndex=!random! %% 36"
    for %%j in (!randomIndex!) do set "aesSeed=!aesSeed!!characters:~%%j,1!"
)

rem ��������� AdditionalOptions ����
set "newOptions=%fixedOptions%%bcfProb% -mllvm -bcf_loop=1 %subOptions%%subLoop% %splitOptions%%splitNum% %aesSeedOptions%%aesSeed%"

echo ���ɵ��� AdditionalOptions: %newOptions%

set "inTag=false"
set "tempFile=temp.vcxproj"

rem �����ʱ�ļ�
type nul > %tempFile%

set "startLine="
for /f "usebackq delims=" %%a in ("%projectFile%") do (
    set "line=%%a"
    if "!inTag!"=="false" (
        set "checkLine=!line!"
        set "checkLine=!checkLine: =!"
        if /i "!checkLine:%searchStart%=!" neq "!checkLine!" (
            echo �ҵ� AdditionalOptions ��ǩ��ʼ����
            set "inTag=true"
            set "startLine=!line!"
            echo !startLine!>> %tempFile%
            echo !newOptions!>> %tempFile%
        ) else (
            echo !line!>> %tempFile%
        )
    ) else (
        set "checkLine=!line!"
        set "checkLine=!checkLine: =!"
        if /i "!checkLine:%searchEnd%=!" neq "!checkLine!" (
            echo �ҵ� AdditionalOptions ��ǩ��������
            set "inTag=false"
            echo !line!>> %tempFile%
        )
    )
)

if "!inTag!"=="true" (
    echo δ�ҵ� AdditionalOptions ��ǩ�������֣������ļ���ʽ����
    pause
    exit /b 1
)

rem �滻ԭ��Ŀ�ļ�
move /y %tempFile% "%projectFile%"

if %errorlevel% neq 0 (
    echo �滻ԭ��Ŀ�ļ�ʱ���ִ��������ļ�Ȩ�ޡ�
    pause
    exit /b %errorlevel%
)

echo �ļ����޸ģ���������������б���...

endlocal