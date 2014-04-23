# ~/.bashrc: executed by bash(1) for non-login shells.
# see /usr/share/doc/bash/examples/startup-files (in the package bash-doc)
# for examples

# If not running interactively, don't do anything
[ -z "$PS1" ] && return

# don't put duplicate lines in the history. See bash(1) for more options
# don't overwrite GNU Midnight Commander's setting of `ignorespace'.
HISTCONTROL=$HISTCONTROL${HISTCONTROL+:}ignoredups
# ... or force ignoredups and ignorespace
HISTCONTROL=ignoreboth

# append to the history file, don't overwrite it
shopt -s histappend

# for setting history length see HISTSIZE and HISTFILESIZE in bash(1)

# check the window size after each command and, if necessary,
# update the values of LINES and COLUMNS.
shopt -s checkwinsize

# make less more friendly for non-text input files, see lesspipe(1)
#[ -x /usr/bin/lesspipe ] && eval "$(SHELL=/bin/sh lesspipe)"

# set variable identifying the chroot you work in (used in the prompt below)
if [ -z "$debian_chroot" ] && [ -r /etc/debian_chroot ]; then
    debian_chroot=$(cat /etc/debian_chroot)
fi

# set a fancy prompt (non-color, unless we know we "want" color)
case "$TERM" in
    xterm-color) color_prompt=yes;;
esac

# uncomment for a colored prompt, if the terminal has the capability; turned
# off by default to not distract the user: the focus in a terminal window
# should be on the output of commands, not on the prompt
force_color_prompt=yes

if [ -n "$force_color_prompt" ]; then
    if [ -x /usr/bin/tput ] && tput setaf 1 >&/dev/null; then
	# We have color support; assume it's compliant with Ecma-48
	# (ISO/IEC-6429). (Lack of such support is extremely rare, and such
	# a case would tend to support setf rather than setaf.)
	color_prompt=yes
    else
	color_prompt=
    fi
fi

if [ "$color_prompt" = yes ]; then
    PS1='${debian_chroot:+($debian_chroot)}\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '
else
    PS1='${debian_chroot:+($debian_chroot)}\u@\h:\w\$ '
fi
unset color_prompt force_color_prompt

# If this is an xterm set the title to user@host:dir
case "$TERM" in
xterm*|rxvt*)
    PS1="\[\e]0;${debian_chroot:+($debian_chroot)}\u@\h: \w\a\]$PS1"
    ;;
*)
    ;;
esac

# enable color support of ls and also add handy aliases
if [ -x /usr/bin/dircolors ]; then
    test -r ~/.dircolors && eval "$(dircolors -b ~/.dircolors)" || eval "$(dircolors -b)"
    alias ls='ls --color=auto'
    #alias dir='dir --color=auto'
    #alias vdir='vdir --color=auto'

    #alias grep='grep --color=auto'
    #alias fgrep='fgrep --color=auto'
    #alias egrep='egrep --color=auto'
fi

# some more ls aliases
alias ll='ls -l'
#alias la='ls -A'
#alias l='ls -CF'

# IgorL's tunning start
alias rm='rm -i'
alias cp='cp -i'
alias mv='mv -i'

export EDITOR=$'gvim -f'
alias jwindows='rdesktop osi -f -p hgfkjh -u jungo -d OSAMADOMAIN -a 16'
alias jgrep='jcvs grep'
alias jdiff='cat qqq | xargs -n1 cvsdiff'

export GREP_COLOR="01;33"
export GREP_OPTIONS="--color=auto"

# complete rt command (thanks for MichaR)
complete -c rt

#export JPKG_AUTO_UPDATE_RND_TOOLS=1
export PATH=$PATH:/usr/sbin/:/sbin
export PS1=$PS1"\[\e]0;\H:\w\a\]"
# IgorL's tunning END

#Irena's tunning
alias ccc='completed.sh $?'
alias callme="ccc ; xmessage -center -default okay make in \`pwd\` is done at \`date\`"
# Alias definitions.
# You may want to put all your additions into a separate file like
# ~/.bash_aliases, instead of adding them here directly.
# See /usr/share/doc/bash-doc/examples in the bash-doc package.

if [ -f ~/.bash_aliases ]; then
    . ~/.bash_aliases
fi

p () { /bin/grep --color=always -i $@ ~/stuff/cisco/users.csv; }
px () { /bin/grep --color=always -iw $@ ~/stuff/cisco/users.csv; }

# enable programmable completion features (you don't need to enable
# this, if it's already enabled in /etc/bash.bashrc and /etc/profile
# sources /etc/bash.bashrc).
#if [ -f /etc/bash_completion ] && ! shopt -oq posix; then
#    . /etc/bash_completion
#fi
#Irena's tunning END

# Source global definitions
if [ -f /etc/bashrc ] ; then
    . /etc/bashrc
fi
# prompt color
export C_RED='\[\033[1;31m\]'
export C_GREEN='\[\033[1;32m\]'
export C_YELLOW='\[\033[1;33m\]'
export C_BLUE='\[\033[1;34m\]'
export C_PURPLE='\[\033[1;35m\]'
export C_CYAN='\[\033[1;36m\]'
export C_WHITE='\[\033[1;37m\]'
export C_BLACK='\[\033[1;38m\]'
export C_DEF='\[\033[0m\]'

C_USER=$C_WHITE
if [ "$UID" = "0" ]; then
    C_USER=$C_RED
fi

#if [ "$TERM" == "linux" ] || [ "$DISPLAY" != ":0.0" ]; then
if [ "$TERM" == "linux" ] || [ "${DISPLAY/:*/ok}" != ok ]; then
  declare PS1="$C_USER\\u@\\h \\W>$C_DEF "
else
  declare PS1="$C_USER\\u \\W>$C_DEF"
#  declare PS1="$C_USER\\u@\\h \\w>$C_DEF \[\e]30;\W\a"
fi

#export PS1='\[\033[01;33m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '

# The following line was automatically added by Jungo tools installation
# process
. /etc/profile_jungo_tools
