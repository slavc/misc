syntax on
filetype on
au BufNewFile,BufRead *.ecpp setlocal filetype=cpp
au BufNewFile,BufRead *.jsp setlocal filetype=xml
colorscheme default
set background=dark
set ic
set incsearch
set nowrap
set expandtab
set softtabstop=4
set shiftwidth=4
set ai
set cindent
set cinoptions=:0
"set hidden " no annoying "no write since last change" message

set wildchar=<Tab> wildmenu wildmode=full
set wildcharm=<C-Z>
nnoremap <Tab> :b <C-Z>
