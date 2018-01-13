[ -f ~/.common_rc.sh ] && source ~/.common_rc.sh || :

export PS1="\e[33m\][\e[32m\]\t \e[33m\]\w]\e[0m\]\n(*'-')<[\$?] \$ "
export PROMPT_DIRTRIM=3

shopt -s autocd
shopt -s globstar

[ -f ~/.fzf.bash ] && source ~/.fzf.bash || :
[ -f ~/.local.bashrc ] && source ~/.local.bashrc || :
