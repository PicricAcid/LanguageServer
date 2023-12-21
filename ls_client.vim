autocmd! BufReadPre *.txt call s:LS_ClientInit()
autocmd! BufReadPost *.txt call s:LS_textOpen()
autocmd! TextChangedI *.txt call s:LS_textChange()
autocmd! ExitPre *.txt call s:LS_textClose()
autocmd! ExitPre *.txt call s:LS_Shutdown()

command! LSclientInit call s:LS_ClientInit()
function! s:LS_ClientInit()
    let s:server_path = 'unix:/tmp/language_server.sock'
    let s:channel = ch_open(s:server_path, #{mode: 'lsp', callback: 'GetDiagnostics'})
    call s:LS_Initialize()
endfunction

function! s:LS_Initialize()
    let req = {}
    let req.method = 'Initialize'
    let req.params = {}
    let reqstatus = ch_evalexpr(s:channel, req, #{timeout: 100})
    if reqstatus->empty()
	echo 'handle error: Initialize'
    else
	if ! reqstatus.result->empty()
	    call s:Callback_Initialize(reqstatus)
	else
	    echo 'communication error: Initialize'
	endif
    endif
endfunction

function! s:Callback_Initialize(res)
    if ! a:res.result.capabilities->empty()
	let s:server_capabilities = a:res.result.capabilities
	echo s:server_capabilities
	call s:LS_Initialized()
    else
	echo 'communication error: Initialize'
    endif
endfunction

function! s:LS_Initialized()
    let channel = ch_open(s:server_path, #{mode: 'lsp'})
    let req = {}
    let req.method = 'Initialized'
    let req.id = 200
    let req.params = {}
    let reqstatus = ch_sendexpr(s:channel, req)
endfunction

command! LStextOpen call s:LS_textOpen()
function! s:LS_textOpen()
    let req = {}
    let req.method = 'textDocument/didOpen'
    let req.id = 200
    let req.params = {}

    let s:change_version = 1 
    let filepath = expand('%')
    
    let filecontent = ""
    for line in readfile(filepath)
	let filecontent = filecontent . line
    endfor
    let req.params.textDocument = #{uri: filepath, languageID: "helloworld", version: s:change_version, text: filecontent}
    let reqstatus = ch_sendexpr(s:channel, req)
endfunction

function! GetDiagnostics(channel, msg)
    let diag = a:msg.params.PublishDiagnosticsParams.diagnostics[0].message
    let message = "Diagnostics: " . diag
    if a:msg.method == 'textDocument/publishDiagnostics' 
	let popup_id = popup_atcursor(message, #{pos: 'topleft'})
	let s:my_popup_id = popup_id
    endif
endfunction

command! LStextChange call s:LS_textChange()
function! s:LS_textChange()
    let req = {}
    let req.method = 'textDocument/didChange'
    let req.params = {}
    let filepath = expand('%')
    let s:change_version = s:change_version + 1
    let req.params.textDocument = #{uri: filepath, version: s:change_version}
    let change_text = s:GetChange()
    let req.params.contentChanges = #{text: change_text}
    let reqstatus = ch_sendexpr(s:channel, req, #{callback: 'GetDiagnostics'})
    echo reqstatus
endfunction

command! -nargs=0 GetChange call s:GetChange()
function! s:GetChange()
    let change_result = execute('changes')
    let change_list = split(change_result, '\n')
    let line = split(change_list[len(change_list) - 2])
    let result = ""
    for i in range(3, len(line) - 1)
	let result = result . " " . line[i]
    endfor
    return result
endfunction

command! LSGetLog call ch_logfile('channellog', 'w')

command! LStextClose call s:LS_textClose()
function! s:LS_textClose()
    let req = {}
    let req.method = 'textDocument/didClose'
    let req.params = {}
    let filepath = expand('%')
    let req.params.textDocument = #{uri: filepath, version: s:change_version}
    let reqstatus = ch_sendexpr(s:channel, req)
endfunction

function! s:LS_Shutdown()
    let req = {}
    let req.method = 'Shutdown'
    let req.params = {}
    let reqstatus = ch_evalexpr(s:channel, req, #{timeout: 100})
    if reqstatus->empty()
	echo 'handle error: Shutdown'
    else
	call s:Callback_Shutdown(reqstatus)
    endif
endfunction

function! s:Callback_Shutdown(res)
    echo a:res
    call s:LS_Exit()
endfunction

function! s:LS_Exit()
    let req = {}
    let req.method = 'Exit'
    let req.id = 200
    let req.params = {}
    let reqstatus = ch_sendexpr(s:channel, req)
    call ch_close(s:channel)
endfunction

command! -nargs=? ChannelDemo call s:ChannelDemo(<f-args>)
function! s:ChannelDemo(...)
    if a:0 >= 1
	if a:1 == 'SHUTDOWN'
	    call s:LS_Shutdown()
	elseif a:1 == 'INITIALIZE'
	    call s:LS_Initialize()
	else
	    echo 'argument error'
	endif
    else
	call s:LS_Initialize()
    endif
endfunction

command! -nargs=0 ChannelDemoTest call s:ChannelDemoTest()
function! s:ChannelDemoTest()
    let channel = ch_open('unix:/tmp/server.sock', #{mode: 'raw'})
    let reqstatus = ch_evalraw(channel, "Hello Wrold from Vim")
    call s:MyHandler(reqstatus)
    call ch_close(channel)
endfunction
