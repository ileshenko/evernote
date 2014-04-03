alias ll='ls -l'
alias cp='cp -i'
alias rm='rm -i'
#alias g='gvim'
g()
{
    gvim.exe "$@" &
}
PATH=/c/Program\ Files/Vim/vim74:$PATH
