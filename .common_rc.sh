alias gcc='gcc -std=gnu11 -Wall -Wextra'
alias g++='g++ -std=gnu++14 -Wall -Wextra'
alias grep='grep --color=auto'
alias ls='ls --color=auto'
alias vsh='vi -c VimShell'
alias gvsh='gvim -c VimShell &'
alias cls='echo -ne "\ec\e[3J"'
alias cp='cp -i'
alias mv='mv -i'
[ -f /usr/local/bin/vim ] && alias vim=/usr/local/bin/vim || :


mkcd() {
  \mkdir $1 && \cd $1
}


easy-compress() {
  easy-compress-usage() {
    echo '[Usage]'
    echo "  $0 [options...] [destination-file] [source-files...]"
    echo '[Option]'
    echo '  -f FORMAT, --format=FORMAT (Default: gzip)'
    echo '    Specify compression format (Only when the argument is one)'
    echo '  -l LEVEL, --level=LEVEL (Default: 6)'
    echo '    Specify compression level'
    echo '  -o FILE, --output=FILE'
    echo '    Specify output file'
    echo '  -p PWSSWORD, --password=PASSWORD'
    echo '    Specify password (zip and 7zip only)'
    echo '  -h, --help'
    echo '    Show help and exit'
  }

  unset GETOPT_COMPATIBLE
  local opt=`getopt -o f:l:o:p:h -l format:,level:,output:,password:,help -- "$@"`
  if [ $? -ne 0 ]; then
    echo >&2 'Failed to parse arguments'
    easy-compress-usage >&2
    unset -f easy-compress-usage
    return 1
  fi
  eval set -- "$opt"

  local format="gzip"
  local level=6
  while [ $# -gt 0 ]; do
    case $1 in
      -f | --format)
        format="$2"
        shift
        ;;
      -l | --level)
        level="$2"
        shift
        ;;
      -h | --help)
        easy-compress-usage
        unset -f easy-compress-usage
        return 0
        ;;
      -o | --output)
        local dstfile="$2"
        shift
        ;;
      -p | --password)
        local password="$2"
        shift
        ;;
      --)
        shift
        break
        ;;
    esac
    shift
  done

  if [ $# -eq 0 ]; then
    echo >&2 'Failed to parse arguments'
    easy-compress-usage >&2
    unset -f easy-compress-usage
    return 1
  elif [ $# -eq 1 ]; then
    local opt_output=`${dstfile+:} false && echo "--output=\"$dstfile\""`
    local opt_password=`${password+:} false && echo "--password=\"$password\""`
    easy-compress-one --format="$format" --level="$level" $opt_output $opt_password $@
    unset -f easy-compress-usage
    return $?
  elif ! ${dstfile+:} false; then
    local dstfile="$1"
    shift
  fi

  local ret=0
  case $dstfile in
    *.tar.gz | *.tar.Z | *.tgz)
      local tmp="$GZIP"; GZIP="-$level"
      tar zcvf "$dstfile" $@
      ret=$?
      GZIP=$tmp
      ;;
    *.tar.bz2 | *.tbz2)
      local tmp="$BZIP2"; BZIP2="-$level"
      tar jcvf "$dstfile" $@
      ret=$?
      BZIP2=$tmp
      ;;
    *.tar.xz | *.txz)
      local tmp="$XZ_OPT"; XZ_OPT="-$level"
      tar Jcvf "$dstfile" $@
      ret=$?
      XZ_OPT=$tmp
      ;;
    *.tar)
      tar cvf "$dstfile" $@
      ret=$?
      ;;
    *.gz)
      if [ $# -gt 1 ]; then
        echo >&2 "Too many files are specified for gzip compression: $@"
        unset -f easy-compress-usage
        return 1
      fi
      gzip "-$level" -c "$1" > "$dstfile"
      ret=$?
      ;;
    *.bz2)
      if [ $# -gt 1 ]; then
        echo >&2 "Too many files are specified for bzip2 compression: $@"
        unset -f easy-compress-usage
        return 1
      fi
      bzip2 "-$level" -c "$1" > "$dstfile"
      ret=$?
      ;;
    *.xz)
      if [ $# -gt 1 ]; then
        echo >&2 "Too many files are specified for xz compression: $@"
        unset -f easy-compress-usage
        return 1
      fi
      xz "-$level" -c "$1" > "$dstfile"
      ret=$?
      ;;
    *.lzh)
      lha av "$dstfile" $@
      ret=$?
      ;;
    *.zip)
      if ${password+:} false; then
        zip "-$level" -e --password=$password -r -v "$dstfile" $@
      else
        zip "-$level" -r -v "$dstfile" $@
      fi
      ret=$?
      ;;
    *.7z)
      if ${password+:} false; then
        7z a -mx="$level" -p"$password" "$dstfile" $@
      else
        7z a -mx="$level" "$dstfile" $@
      fi
      ret=$?
      ;;
    *)
      echo >&2 "Cannot compress files to $dstfile: unrecognized compression format"
      unset -f easy-compress-usage
      return 1
      ;;
  esac
  unset -f easy-compress-usage
  return $ret
}


easy-compress-one() {
  easy-compress-one-usage() {
    echo '[Usage]'
    echo "  $0 [options...] [source-directory]"
    echo '[Option]'
    echo '  -f FORMAT, --format=FORMAT (Default: gzip)'
    echo '    Specify compression format'
    echo '  -l LEVEL, --level=LEVEL (Default: 6)'
    echo '    Specify compression level'
    echo '  -o FILE, --output=FILE'
    echo '    Specify output file'
    echo '  -p PWSSWORD, --password=PASSWORD'
    echo '    Specify password (zip and 7zip only)'
    echo '  -h, --help'
    echo '    Show help and exit'
  }

  unset GETOPT_COMPATIBLE
  local opt=`getopt -o f:l:o:p:h -l format:,level:,output:,password:,help -- "$@"`
  if [ $? -ne 0 ]; then
    echo >&2 'Failed to parse arguments'
    easy-compress-one-usage >&2
    unset -f easy-compress-one-usage
    return 1
  fi
  eval set -- "$opt"

  local format="gzip"
  local level=6
  while [ $# -gt 0 ]; do
    case $1 in
      -f | --format)
        format="$2"
        shift
        ;;
      -l | --level)
        level="$2"
        shift
        ;;
      -o | --output)
        local dstfile="$2"
        shift
        ;;
      -h | --help)
        easy-compress-one-usage
        unset -f easy-compress-one-usage
        return 0
        ;;
      -p | --password)
        local password="$2"
        shift
        ;;
      --)
        shift
        break
        ;;
    esac
    shift
  done

  if [ $# -eq 0 ]; then
    echo >&2 'No item is specified'
    unset -f easy-compress-one-usage
    return 1
  elif [ $# -gt 1 ]; then
    echo >&2 'Too many items are specified'
    unset -f easy-compress-one-usage
    return 1
  fi

  local ret=0
  if [ -d $1 ]; then
    case "$format" in
      gzip | tgz)
        ${dstfile+:} false || local dstfile=${1%/}.tgz
        local tmp="$GZIP"; GZIP="-$level"
        tar zcvf "$dstfile" $1
        ret=$?
        GZIP=$tmp
        ;;
      bzip2 | tbz2)
        ${dstfile+:} false || local dstfile=${1%/}.tbz2
        local tmp="$BZIP2"; BZIP2="-$level"
        tar jcvf "$dstfile" $1
        ret=$?
        BZIP2=$tmp
        ;;
      xz | txz)
        ${dstfile+:} false || local dstfile=${1%/}.txz
        local tmp="$XZ_OPT"; XZ_OPT="-$level"
        tar Jcvf "$dstfile" $1
        ret=$?
        XZ_OPT=$tmp
        ;;
      lha | lzh)
        ${dstfile+:} false || local dstfile="${1%/}.lzh"
        lha av "$dstfile" "$1"
        ret=$?
        ;;
      zip)
        ${dstfile+:} false || local dstfile="${1%/}.zip"
        if ${password+:} false; then
          zip "-$level" -e --password=$password -r -v "$dstfile" $1
        else
          zip "-$level" -r -v "$dstfile" $1
        fi
        ret=$?
        ;;
      7zip | 7z)
        ${dstfile+:} false || local dstfile="${1%/}.7z"
        if ${password+:} false; then
          7z a -mx="$level" -p"$password" "$1"
        else
          7z a -mx="$level" "$1"
        fi
        ret=$?
        ;;
      *)
        echo >&2 "Unable to recognize compression format: $format"
        unset -f easy-compress-one-usage
        return 1
        ;;
    esac
  elif [ -f $1 ]; then
    case "$format" in
      gzip | gz)
        ${dstfile+:} false || local dstfile="$1.gz"
        gzip "-$level" -c "$1" > "$dstfile"
        ret=$?
        ;;
      bzip2 | tgz)
        ${dstfile+:} false || local dstfile="$1.bz2"
        bzip2 "-$level" -c "$1" > "$dstfile"
        ret=$?
        ;;
      xz)
        ${dstfile+:} false || local dstfile="$1.xz"
        xz "-$level" -c "$1" > "$dstfile"
        ret=$?
        ;;
      lha | lzh)
        ${dstfile+:} false || local dstfile="$1.lzh"
        lha av "$dstfile" "$1"
        ret=$?
        ;;
      zip)
        ${dstfile+:} false || local dstfile="$1.zip"
        if ${password+:} false; then
          zip "-$level" -e --password=$password -v "$dstfile" $1
        else
          zip "-$level" -v "$dstfile" "$1"
        fi
        ret=$?
        ;;
      7zip | 7z)
        ${dstfile+:} false || local dstfile="$1.7z"
        if ${password+:} false; then
          7z a -mx="$level" -p"$password" "$1"
        else
          7z a -mx="$level" "$1"
        fi
        ret=$?
        ;;
      *)
        echo >&2 "Unable to recognize compression format: $format"
        unset -f easy-compress-one-usage
        return 1
        ;;
    esac
  fi
  unset -f easy-compress-one-usage
  return $ret
}


easy-extract() {
  easy-extract-usage() {
    echo '[Usage]'
    echo "  $0 [options...] [source-directory]"
    echo '[Option]'
    echo '  -p PWSSWORD, --password=PASSWORD'
    echo '    Specify password (zip and 7zip only)'
    echo '  -h, --help'
    echo '    Show help and exit'
  }

  unset GETOPT_COMPATIBLE
  local opt=`getopt -o p:h -l password:,help -- "$@"`
  if [ $? -ne 0 ]; then
    echo >&2 'Failed to parse arguments'
    easy-extract-usage >&2
    unset -f easy-extract-usage
    return 1
  fi
  eval set -- "$opt"

  while [ $# -gt 0 ]; do
    case $1 in
      -h | --help)
        easy-extract-usage
        unset -f easy-extract-usage
        return 0
        ;;
      -p | --password)
        local password="$2"
        shift
        ;;
      --)
        shift
        break
        ;;
    esac
    shift
  done

  if [ $# -eq 0 ]; then
    echo >&2 'No item is specified'
    unset -f easy-extract-usage
    return 1
  elif [ $# -gt 1 ]; then
    echo >&2 "Too many items are specified: $@"
    unset -f easy-extract-usage
    return 1
  fi

  local base="${1%.*}"
  local ret=0
  case $1 in
    *.tar.gz | *.tar.Z | *.tgz)
      tar zxvf "$1"
      ret=$?
      ;;
    *.tar.bz2 | *.tbz2)
      tar jxvf "$1"
      ret=$?
      ;;
    *.tar.xz | *.txz)
      tar Jxvf "$1"
      ret=$?
      ;;
    *.tar)
      tar xvf "$1"
      ret=$?
      ;;
    *.gz)
      gzip -dc "$1" > "$base"
      ret=$?
      ;;
    *.bz2)
      bzip2 -dc "$1" > "$base"
      ret=$?
      ;;
    *.xz)
      xz -dc "$1" > "$base"
      ret=$?
      ;;
    *.lzh)
      lha xv "$1"
      ret=$?
      ;;
    *.zip)
      if ${password+:} false; then
        unzip -P $password "$1"
      else
        unzip "$1"
      fi
      ret=$?
      ;;
    *.7z)
      if ${password+:} false; then
        7z x -p"$password" "$1"
      else
        7z x "$1"
      fi
      ret=$?
      ;;
    *)
      echo "Cannot extract files from $1 - unrecognized compression format" 1>&2
      return 1
      ;;
  esac
  unset -f easy-extract-usage
  return $ret
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
}


case ${OSTYPE} in
  cygwin | msys)
    open-browser() {
      rundll32 url.dll,FileProtocolHandler $1
    }
    ;;
  darwin*)
    open-browser() {
      open -a Google\ Chrome $1
    }
    ;;
  linux*)
    open-browser() {
      xdg-open $1
    }
    ;;
esac


google() {
  local str opt
  if [ $# != 0 ]; then
    for i in $*; do
      str="$str${str:++}$i"
    done
    opt='search?num=100'
    opt="${opt}&q=${str}"
  fi
  open-browser "http://www.google.co.jp/$opt"
}
