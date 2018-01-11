#!/bin/sh -eu

targets=(
  .common_profile.sh
  .common_rc.sh
  .bash_profile
  .bashrc
  .zprofile
  .zshrc
  .inputrc
)


script_dir=$(cd $(dirname $0); pwd)
show_usage() {
  echo '[Usage]'
  echo "  $0 [options...] [arguments...]"
  echo '[Option]'
  echo '  -a, --append'
  echo '    Not to replace, append to the config file which is already exists'
  echo '  -c, --copy'
  echo '    Copy or replace config file'
  echo '  -d, --destination-dir'
  echo '    Specify destnation dir'
  echo '  -s, --symbolic'
  echo '    Create symbolic link instead of copying'
  echo '  -h, --help'
  echo '    Show help and exit'
}


unset GETOPT_COMPATIBLE
OPT=`getopt -o acd:sh -l apped,copy,destination-dir:,symbolic,help -- "$@"`
if [ $? -ne 0 ]; then
  echo >&2 'Invalid argument'
  show_usage >&2
  exit 1
fi
eval set -- "$OPT"

dst_dir=~
mode=copy

while [ $# -gt 0 ]; do
  case $1 in
    -a | --append)
      mode=append
      ;;
    -c | --copy)
      mode=copy
      ;;
    -d | --destination-dir)
      dst_dir="$2"
      shift
      ;;
    -s | --symbolic)
      mode=symbolic
      shift;;
    -h | --help)
      show_usage
      exit 0
      ;;
    --)
      shift
      break
      ;;
  esac
  shift
done

[ -d $dst_dir ] && : || mkdir -p $dst_dir

case "$mode" in
  append)
    for target in ${targets[@]}; do
      if [ -f $dst_dir/$target ]; then
        echo -n "$dst_dir/$target is already exists. Append to it? [y/n]"
        line=''
        while [ "$line" != 'n' ]; do
          read line
          if [ "$line" == 'y' ]; then
            echo >> $dst_dir/$target
            cat $script_dir/$target >> $dst_dir/$target
            break
          fi
        done
      else
        cat $script_dir/$target >> $dst_dir/$target
      fi
    done
    ;;
  copy)
    for target in ${targets[@]}; do
      cp -i $script_dir/$target $dst_dir/$target
    done
    ;;
  symbolic)
    for target in ${targets[@]}; do
      ln -s $script_dir/$target $dst_dir/$target
    done
    ;;
  *)
    echo "$mode Didn't match anything"
esac
