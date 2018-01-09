[ -f ~/.common_rc.sh ] && source ~/.common_rc.sh || :

export PS1="\e[33m\][\e[32m\]\d \t \e[33m\]\w]\e[0m\]\n(*'-')<[\$?] \$ "

shopt -s autocd
shopt -s globstar

[ -f ~/.fzf.bash ] && source ~/.fzf.bash || :
[ -f ~/.local.bashrc ] && source ~/.local.bashrc || :
