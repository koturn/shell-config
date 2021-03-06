#!/bin/sh -eu

targets=" \
  .common_profile.sh \
  .common_rc.sh \
  .bash_profile \
  .bashrc \
  .zprofile \
  .zshrc \
  .inputrc"
target_list=`echo $targets | cut -f 1- -d ' '`

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
  echo '  -f, --force'
  echo '    Force to append, replace or create symolic link'
  echo '  -s, --symbolic'
  echo '    Create symbolic link instead of copying'
  echo '  -h, --help'
  echo '    Show help and exit'
}


unset GETOPT_COMPATIBLE
OPT=`getopt -o acd:fsh -l apped,copy,destination-dir:,force,symbolic,help -- "$@"`
if [ $? -ne 0 ]; then
  echo >&2 'Invalid argument'
  show_usage >&2
  exit 1
fi
eval set -- "$OPT"

dst_dir=~
mode=copy
is_forced=0

while [ $# -gt 0 ]; do
  case $1 in
    -a | --append)
      mode=append
      ;;
    -c | --copy)
      mode=copy
      ;;
    -f | --force)
      is_forced=1
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
    for target in $target_list; do
      if [ -f $dst_dir/$target -a $is_forced -ne 0 ]; then
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
    if [ $is_forced -eq 1 ]; then
      for target in $target_list; do
        cp $script_dir/$target $dst_dir/$target
      done
    else
      for target in $target_list; do
        cp -i $script_dir/$target $dst_dir/$target
      done
    fi
    ;;
  symbolic)
    for target in $target_list; do
      ln -s $script_dir/$target $dst_dir/$target
    done
    ;;
  *)
    echo "$mode Didn't match anything"
esac
