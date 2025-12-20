[ -f ~/.common_rc.sh ] && source ~/.common_rc.sh || :

# Set color of ls (change $LS_COLORS)
eval "$(dircolors -b)"


###############################################################################
# Prompt config                                                               #
###############################################################################
autoload colors
colors

autoload -Uz VCS_INFO_get_data_git; VCS_INFO_get_data_git 2> /dev/null
setopt prompt_subst
rprompt-git-current-branch() {
  local name action color
  [[ "${PWD}" =~ '/\.git/?' ]] && return || :
  name=${"$(git symbolic-ref HEAD 2> /dev/null || git tag --points-at HEAD 2> /dev/null | head -1)"##*/} || return
  action=$(VCS_INFO_git_getaction "$(git rev-parse --git-dir 2> /dev/null)") && action="(${action})"
  case "$(git status 2> /dev/null | tail -1)" in
    'nothing to'*) color=%F{green} ;;
    'nothing added'*) color=%F{yellow} ;;
    '# Untracked'*) color=%B%F{red} ;;
    *) color=%F{red} ;;
  esac
  echo "${color}${name}${action}%f%b "
}

PROMPT="%(?.%B%F{magenta}(*'-'.%B%F{red}(;_;))%(?..<[\$?]) %(!.#.$) %f%b"
PROMPT2="%B%F{magenta}%_> %f%b"
RPROMPT='%B%F{yellow}[%f%b%D{%K:%M:%S} $(rprompt-git-current-branch)%B%F{yellow}%~]%f%b'
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
chpwd() {
  ls --color=auto
  git rev-parse 2> /dev/null && {
    RPROMPT='%B%F{yellow}[%f%b%D{%K:%M:%S} $(rprompt-git-current-branch)%B%F{yellow}%~]%f%b'
  } || {
    RPROMPT='%B%F{yellow}[%f%b%D{%K:%M:%S} %B%F{yellow}%~]%f%b'
  }
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


bindkey "^[OH" beginning-of-line
bindkey "^[OF" end-of-line
bindkey "^[[3~" delete-char

[ -f ~/.fzf.zsh ] && source ~/.fzf.zsh || :
[ -f ~/.local.zshrc ] && source ~/.local.zshrc || :
