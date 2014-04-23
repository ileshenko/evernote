"set foldmethod=indent
"set foldcolumn=0
"set foldlevel=5

"set ic
set nobackup
set sm
set is

let g:dont_load_funcheader = 1 

au BufRead */asterisk/*.[ch] set ts=4 columns=120 nowrap

set cino=:0,(s,u0,U1,g0,t0,*100

set lines=45 columns=80
"set guifont=Fixed\ 12

map <S-Insert> <MiddleMouse>
map! <S-Insert> <MiddleMouse>

map <f1> :cs add cscope.out<cr>:set cst<cr>
map <f2> :execute ':!cvsdiff' TokenUnderCursor(1) '&' <cr><cr>
map <silent> <f3> /^.\{81,\}\\|^\t.\{73,\}\\|^\t\t.\{65,\}<cr>
map <silent> <s-f3> /^\s*\n\s*$<cr>
"map <f4> :%s/ethernet frame size:// <cr> :%s/\n.*ed:// <cr> :%s/pa.*ec:/-/ <cr>
"map <f4> :set ts=4 nowrap columns=120<cr>
"map <f4> :execute ':!xxdiff /tmp/ikan_dev/' . TokenUnderCursor(1) '~/rg/5_1.commit/vendor/ikanos/' . TokenUnderCursor(1) '&' <cr><cr>
map <f4> :execute ':!cvs stat ' TokenUnderCursor(1)  <cr>

let g:dont_load_fuzzyfinder = 1
map <f5> :execute ':!cvsdiff -r ugw-4_2-ga -r ugw-5_1-linux' TokenUnderCursor(1) '&' <cr><cr>

noremap <f6> :execute ':!cvsdiff -r ugw-4_2-ga -r branch-5_4' TokenUnderCursor(1) '&' <cr><cr>
"noremap <f6> :execute ':!cvsdiff -r 1.1 -r `jcvs revision ' TokenUnderCursor(1) '` ' TokenUnderCursor(1) '&' <cr><cr>

"noremap <f6> :execute ':!cvsdiff -r 1.1 -r `jcvs repository' ' 1.2` ' TokenUnderCursor(1) '&' <cr><cr>

"noremap <f5> :execute ':!xxdiff ' TokenUnderCursor(1) '/tmp/pirelli/pirelli.vdsl.cvs/rg/' . TokenUnderCursor(1) '&' <cr><cr>
"noremap <f6> :execute ':!cvsdiff -r adi-bsp-milestone-5_0_0 -r branch-4_11_3_9' TokenUnderCursor(1) '&' <cr><cr>
"noremap <f6> :execute ':!cvsdiff -r conexant-solos-bsp-1_1 -r tag-4_1_8_3_6' TokenUnderCursor(1) '&' <cr><cr>
noremap <f7> :execute ':!cvsdiff -r 1 ' TokenUnderCursor(1) '&' <cr><cr>
noremap <f8> :execute ':!xxdiff ' TokenUnderCursor(1) . '.cnf  ' TokenUnderCursor(1) '&' <cr><cr>

"map <f5> i#define PATH_THROW<cr><esc>jI#undef PATH_THROW<cr>#undef CHARM_TIMER_FREQUENCY<cr>#define CHARM_TIMER_FREQUENCY (8000000)<cr><esc>:wq<cr>

"map <f6> n0kwwv$hhv
" functions
function! IsTokenChar(is_fname, char)
	if a:char == "_"
		return 1
	endif
	if a:char >= "0" && a:char <= "9"
		return 1
	endif
	if a:char >= "A" && a:char <= "Z"
		return 1
	endif
	if a:char >= "a" && a:char <= "z"
		return 1
	endif
	if a:is_fname && (a:char == "/" || a:char == "." || a:char == "-")
		return 1
	endif
	return 0
endfunction

function! TokenUnderCursor(is_fname)
	echo ""
	let line = getline(".")
	let pos = col(".") - 1

	let begpos = pos -1
	while IsTokenChar(a:is_fname, strpart(line, begpos, 1)) == 1
		let begpos = begpos - 1
	endwhile
	
	let endpos = pos + 1
	while IsTokenChar(a:is_fname, strpart(line, endpos, 1)) == 1
		let endpos = endpos + 1
	endwhile

	return strpart(line, begpos+1, endpos-begpos-1)
endfunction

function! Perform()
	:%s/ethernet frame size://
	:%s/\n.*ed://
	:%s/pa.*ec:/-/
	return 0
endfunction
