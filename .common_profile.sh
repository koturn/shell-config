export LANG=ja_JP.UTF-8
case $UID in
  0)
    LANG=C
    ;;
esac

export LESS_TERMCAP_mb=$'\e[1;31m'
export LESS_TERMCAP_md=$'\e[1;32m'
export LESS_TERMCAP_me=$'\e[0m'
export LESS_TERMCAP_se=$'\e[0m'
export LESS_TERMCAP_so=$'\e[1;44;33m'
export LESS_TERMCAP_ue=$'\e[0m'
export LESS_TERMCAP_us=$'\e[4;36m'

export VISUAL=vim
export EDITOR=vim

case $OSTYPE in
  cygwin | msys)
    alias clear='echo -ne "\ec\e[3J"'
    export LIBRARY_PATH=$LIBRARY_PATH:/usr/local/lib/
    export CHERE_INVOKING=1
    export CYGWIN="$CYGWIN error_start=dumper"
    if [ "$OSTYPE" = "msys" ]; then
      if [ "$PROCESSOR_ARCHITECTURE" = "AMD64" -a "$MSYSTEM" != "MINGW64" ]; then
        export PATH=$PATH:/mingw64/bin/:/c/alias/
        export LIBRARY_PATH=$PATH:/mingw64/lib/
        export LD_LIBRARY_PATH=$PATH:/mingw64/bin/
      elif [ "$PROCESSOR_ARCHITECTURE" = "x86" -a "$MSYSTEM" != "MINGW32" ]; then
        export PATH=$PATH:/mingw32/bin/:/c/alias/
        export LIBRARY_PATH=$PATH:/mingw32/lib/
        export LD_LIBRARY_PATH=$PATH:/mingw32/bin/
      fi
    else
      export PATH=$PATH:/c/alias/
    fi
    ;;
esac


[ -f ~/.local.common_profile.sh ] && . ~/.local.common_profile.sh || :
