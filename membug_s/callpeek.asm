; assmbler file for use with Watcom C/C++
; Assumes Watcoms calling convention which uses EAX, EDX, EBX in that order
; for passing parameters
.386p

callpeek_TEXT segment para public use32 'CODE'
                assume cs:callpeek_TEXT
                assume ds:nothing
                
`W?$nwn(ui)pnv`: mov edx,[esp]
                jmp dbg_new_
                
`W?$dln(pnv)v`: mov edx,[esp]
                jmp dbg_delete_

`W?$nan(ui)pnv`: mov edx,[esp]
                jmp dbg_new_array_

`W?$dan(pnv)v`: mov edx,[esp]
                jmp dbg_delete_array_

malloc_:        mov edx,[esp]
                jmp dbg_malloc_
                
free_:          mov edx,[esp]
                jmp dbg_free_

realloc_:       mov ebx,[esp]
                jmp dbg_realloc_

calloc_:        mov ebx,[esp]
                jmp dbg_calloc_

heap_panic_     proc near
                int 3
                ret
heap_panic_     endp

                PUBLIC	`W?$nwn(ui)pnv`
                PUBLIC	`W?$dln(pnv)v`
                PUBLIC	`W?$nan(ui)pnv`
                PUBLIC	`W?$dan(pnv)v`
                PUBLIC	malloc_
                PUBLIC	free_
                PUBLIC	realloc_
                PUBLIC  calloc_
                PUBLIC  heap_panic_
                
                extrn dbg_new_:near
                extrn dbg_delete_:near
                extrn dbg_new_array_:near
                extrn dbg_delete_array_:near
                extrn dbg_malloc_:near
                extrn dbg_free_:near
                extrn dbg_realloc_:near
                extrn dbg_calloc_:near

callpeek_TEXT    ends

                end
