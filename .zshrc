# set language
export LANG=ja_JP.UTF-8
case ${UID} in
  0)
    LANG=C
    ;;
esac

export VISUAL=vim
export EDITOR=vim

# Set color of ls (change $LS_COLORS)
eval "$(dircolors -b)"


###############################################################################
# Prompt config                                                               #
###############################################################################
autoload colors
colors

autoload -Uz VCS_INFO_get_data_git; VCS_INFO_get_data_git 2> /dev/null
setopt prompt_subst
function rprompt-git-current-branch {
  local name st color gitdir action
  if [[ "$PWD" =~ '/\.git(/.*)?$' ]]; then
    return
  fi
  name=$(basename "`git symbolic-ref HEAD 2> /dev/null`")
  if [[ -z $name ]]; then
    return
  fi
  gitdir=`git rev-parse --git-dir 2> /dev/null`
  action=`VCS_INFO_git_getaction "$gitdir"` && action="($action)"
  st=`git status 2> /dev/null`
  if [[ -n `echo "$st" | grep "^nothing to"` ]]; then
    color=%F{green}
  elif [[ -n `echo "$st" | grep "^nothing added"` ]]; then
    color=%F{yellow}
  elif [[ -n `echo "$st" | grep "^# Untracked"` ]]; then
    color=%B%F{red}
  else
    color=%F{red}
  fi
  echo "$color$name$action%f%b "
}

PROMPT="%(?.%B%F{magenta}(*'-'.%B%F{red}(;_;))%(?..<[\$?]) %(!.#.$) %f%b"
PROMPT2="%B%F{magenta}%_> %f%b"
RPROMPT='%B%F{yellow}[%f%b%D{%K:%M:%S} `rprompt-git-current-branch`%B%F{yellow}%~]%f%b'
SPROMPT="%B%F{red} correct: %R -> %r [n,y,a,e]? %f%b"

# Terminal title
case "${TERM}" in
  kterm* | xterm*)
    precmd() {
      echo -ne "\033]0;${USER}@${HOST}\007"
    }
    ;;
esac

# History
HISTFILE=~/.zsh_history
HISTSIZE=1000
SAVEHIST=1000
setopt appendhistory
setopt hist_ignore_all_dups
setopt hist_ignore_space
setopt share_history

# Filter history searching(<C-p>, <C-n>)
bindkey -e
autoload history-search-end
zle -N history-beginning-search-backward-end history-search-end
zle -N history-beginning-search-forward-end history-search-end
bindkey "^p" history-beginning-search-backward-end
bindkey "^n" history-beginning-search-forward-end

# Completion
autoload -U compinit
compinit
setopt complete_aliases
zstyle ':completion:*:default' list-colors ${LS_COLORS}

#----- 手動で設定する場合
#zstyle ':completion:*' list-colors 'di=01;34:ln=01;36:ex=01;32:*.tar=01;31:*.gz=01;31' # 補完候補を色付きで表示
#-----
zstyle ':completion:*:default' menu select=1 # 補完候補をEmacsのキーバインドで選択
zstyle ':completion:*:*:kill:*:processes' list-colors '=(#b) #([%0-9]#)*=0=01;31' # killの候補を色付き表示
zstyle ':completion:*' matcher-list 'm:{a-z}={A-Z} r:|[-_.]=**' # 大文字を補完、-_.の前に*を補うようにして補完
setopt auto_list         # TAB一回目で補完候補を一覧表示
setopt complete_in_word  # 単語の途中でもカーソル位置で置換
setopt list_packed       # 補完候補を詰めて表示
setopt list_types        # 補完候補表示時にファイルの種類を表示。*, @, /

### Moving directory ###
# execute ls automatically after moving directory
function chpwd() {
  ls --color=auto
}
setopt autocd             # Move directory by directory name only, withput "cd"
setopt autopushd          # 自動でpushdする。cd -[tab]で候補表示
setopt chase_links        # リンクへ移動するとき実際のディレクトリへ移動
setopt pushd_ignore_dups  # 重複するディレクトリは記憶しない

# Job
setopt NOBGNICE           # バックグランドのジョブのスピードを落とさない
setopt NOHUP              # ログアウトしてもバックグランドジョブを続ける

# Other
setopt nobeep
setopt brace_ccl     # ブレース展開において{}の中身をソートして展開、また、{a-z}で文字範囲指定
setopt correct       # コマンドのスペルミスを指摘して直す
setopt extendedglob  # 正規表現のなんか？
setopt nomatch       # マッチしない場合はエラー？
setopt notify        # ジョブが終了したらただちに知らせる
setopt rm_star_wait  # rm * を実行する前に確認


###############################################################################
# Aliases                                                                     #
###############################################################################
alias gcc='gcc -std=gnu11 -Wall -Wextra'
alias g++='g++ -std=gnu++14 -Wall -Wextra'
alias grep='grep --color=auto'
alias ls='ls --color=auto'
alias la='ls -A'
alias lf='ls -F'
alias ll='ls -l'
alias l='ls --color=auto'
alias vsh='vi -c VimShell'
alias gvsh='gvim -c VimShell'
alias cls='echo -ne "\ec\e[3J"'


case ${OSTYPE} in
  cygwin)
    alias clear='echo -ne "\ec\e[3J"'
    ;;
esac

# ホストごとの設定があれば読み込む
# [ -f ~/.zshrc.mine ] && source ~/.zshrc.mine


###############################################################################
# Functions                                                                   #
###############################################################################
man() {
  LESS_TERMCAP_mb=$'\e[1;31m' \
    LESS_TERMCAP_md=$'\e[1;32m' \
    LESS_TERMCAP_me=$'\e[0m' \
    LESS_TERMCAP_se=$'\e[0m' \
    LESS_TERMCAP_so=$'\e[1;44;33m' \
    LESS_TERMCAP_ue=$'\e[0m' \
    LESS_TERMCAP_us=$'\e[4;36m' \
    command man "$@"
}

# http://rcmdnk.github.io/blog/2015/07/22/computer-bash-zsh/
explain() {
  if [ "$#" -eq 0 ]; then
    while read  -p 'Command: ' cmd; do
      curl -Gs "https://www.mankier.com/api/explain/?cols="$(tput cols) --data-urlencode "q=$cmd"
    done
    echo 'Bye!'
  elif [ "$#" -eq 1 ]; then
    curl -Gs "https://www.mankier.com/api/explain/?cols="$(tput cols) --data-urlencode "q=$1"
  else
    echo 'Usage'
    echo 'explain                  interactive mode.'
    echo 'explain 'cmd -o | ...'   one quoted command to explain it.'
  fi
}

mkdircd() {
  mkdir -p "$@" && eval cd "\"\$$#\"";
}

git-init-commit() {
  git init . && git add . && git commit -m 'Initial commit'
}

github-first-push() {
  if [ $# -lt 1 ]; then
    echo 'Invalid arguments' 1>&2
    echo '[USAGE]'
    echo "  $0 REMOTE-REPOSITORY-NAME"
    return 1
  fi
  git remote add origin "https://github.com/koturn/$1.git" && \
  git push -u origin master
}

github-clone() {
  if [ $# -lt 1 ]; then
    echo 'Invalid arguments' 1>&2
    echo '[USAGE]'
    echo "  $0 USER-NAME REPOSITORY-NAME"
    return 1
  fi
  git clone "https://github.com/$1/$2.git"
}

git-commit-chmod() {
  find . -type d -name '.git' -prune -o -type d -exec chmod 755 {} \;
  find . -type d -name '.git' -prune -o -type f -exec chmod 644 {} \;
  # chmod 755 **/*.{lua,pl,py,rb,sh}
}


easy-compress() {
  if [ $# -lt 1 ]; then
    echo 'Invalid arguments' 1>&2
    echo '[USAGE]'
    echo "  $0 SRC [Compressed file] {[Destination directory]}"
    return 1
  fi

  local dir=${2-/tmp}
  case $1 in
    *.tar.gz | *.tar.Z)
      tar zxvf "$1" -C "$dir"
      ;;
    *.tar.bz2)
      tar jxvf "$1" -C "$dir"
      ;;
    *.tar.xz)
      tar Jxvf "$1" -C "$dir"
      ;;
    *.zip)
      unzip "$1" -d "$dir"
      ;;
    *)
      echo "Cannot extract files from $1 - unrecognized compression format" 1>&2
      return 1
      ;;
  esac
}


easy-extract() {
  if [ $# -lt 1 ]; then
    echo 'Invalid arguments' 1>&2
    echo '[USAGE]'
    echo "  $0 SRC [Compressed file] {[Destination directory]}"
    return 1
  fi

  local dir=${2-/tmp}
  case $1 in
    *.tar.gz | *.tar.Z)
      tar zxvf "$1" -C "$dir"
      ;;
    *.tar.bz2)
      tar jxvf "$1" -C "$dir"
      ;;
    *.tar.xz)
      tar Jxvf "$1" -C "$dir"
      ;;
    *.zip)
      unzip "$1" -d "$dir"
      ;;
    *)
      echo "Cannot extract files from $1 - unrecognized compression format" 1>&2
      return 1
      ;;
  esac
}
