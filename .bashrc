# set language
export LANG=ja_JP.UTF-8
case ${UID} in
  0)
    LANG=C
    ;;
esac

export VISUAL=vim
export EDITOR=vim
PS1="\e[33m\][\e[32m\]\d \t \e[33m\]\w]\e[0m\]\n(*'-')<[\$?] \$ "
