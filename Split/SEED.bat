@echo off
setlocal enabledelayedexpansion

rem 定义项目文件路径，需根据实际情况修改
set "projectFile=C:\Users\zheng\Desktop\项目\Split\Split\Split.vcxproj"
rem 定义要查找的 AdditionalOptions 标签起始和结束部分
set "searchStart=<AdditionalOptions>"
set "searchEnd=</AdditionalOptions>"
rem 定义固定的 AdditionalOptions 部分
set "fixedOptions=-mllvm -bcf -mllvm -bcf_prob="
set "subOptions=-mllvm -sub -mllvm -sub_loop="
set "splitOptions=-mllvm -split_num="
set "aesSeedOptions=-mllvm -aesSeed="

rem 生成随机的 bcf_prob 值，范围 1 到 30
set /a "bcfProb=!random! %% 15 + 5"
rem 生成随机的 sub_loop 值，范围 0 到 3
set /a "subLoop=!random! %% 1 + 1"
rem 生成随机的 split_num 值，范围 0 到 3
set /a "splitNum=!random! %% 1 + 1"

rem 定义 aesSeed 的长度
set "aesSeedLength=32"
rem 定义可用字符集
set "characters=0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"

rem 生成随机的 aesSeed 值
set "aesSeed="
for /l %%i in (1,1,%aesSeedLength%) do (
    set /a "randomIndex=!random! %% 36"
    for %%j in (!randomIndex!) do set "aesSeed=!aesSeed!!characters:~%%j,1!"
)

rem 组合完整的 AdditionalOptions 内容
set "newOptions=%fixedOptions%%bcfProb% -mllvm -bcf_loop=1 %subOptions%%subLoop% %splitOptions%%splitNum% %aesSeedOptions%%aesSeed%"

echo 生成的新 AdditionalOptions: %newOptions%

set "inTag=false"
set "tempFile=temp.vcxproj"

rem 清空临时文件
type nul > %tempFile%

set "startLine="
for /f "usebackq delims=" %%a in ("%projectFile%") do (
    set "line=%%a"
    if "!inTag!"=="false" (
        set "checkLine=!line!"
        set "checkLine=!checkLine: =!"
        if /i "!checkLine:%searchStart%=!" neq "!checkLine!" (
            echo 找到 AdditionalOptions 标签起始部分
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
            echo 找到 AdditionalOptions 标签结束部分
            set "inTag=false"
            echo !line!>> %tempFile%
        )
    )
)

if "!inTag!"=="true" (
    echo 未找到 AdditionalOptions 标签结束部分，可能文件格式有误。
    pause
    exit /b 1
)

rem 替换原项目文件
move /y %tempFile% "%projectFile%"

if %errorlevel% neq 0 (
    echo 替换原项目文件时出现错误，请检查文件权限。
    pause
    exit /b %errorlevel%
)

echo 文件已修改，按任意键继续进行编译...

endlocal