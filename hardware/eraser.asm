; 
;  Copyright (C) 2012 The Android Open Source Project
; 
;  Licensed under the Apache License, Version 2.0 (the "License");
;  you may not use this file except in compliance with the License.
;  You may obtain a copy of the License at
; 
;       http://www.apache.org/licenses/LICENSE-2.0
;
;  Unless required by applicable law or agreed to in writing, software
;  distributed under the License is distributed on an "AS IS" BASIS,
;  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;  See the License for the specific language governing permissions and
;  limitations under the License.

; ADK2 Sam3X Eraser Controller
; Author: Fuller

; serial DTR line as clock on GP0
; serial RTS line as data on GP1
; GP2 output controls Sam3X erase feature
; GP0, input, /DTR
; GP1, input, /RTS
; GP2, output, erase
; GP3, input, internal pull-up, floating

    #include "P10F200.INC"
; configuration bits
    __CONFIG (_WDT_OFF&_CP_ON&_MCLRE_OFF)

; memory map
; only 16 bytes of RAM starting at 0x10
    udata
GPIO_sampled    res 1
GPIO_sampledQ   res 1
magicKeyL       res 1
magicKeyH       res 1
tick0           res 1
tick1           res 1

    code        0x00
; initialize microcontroller
        movlw   (1<<NOT_GPWU|1<<PS2|1<<PS1|1<<PS0)  ; no wake up, pull-ups
                                             ; TMR0 at 1:256 for low power
        option
        bcf     GPIO, GP2
        movlw   (1<<GP3|1<<GP1|1<<GP0)    ; GP2 is only output
        tris    GPIO

; initialize variables
        clrf    magicKeyH
        clrf    magicKeyL
        bsf     GPIO_sampled, GP0         ; can't get false rise if assume high

sampler:
        movf    GPIO_sampled, W           ; old stale sample
        movwf   GPIO_sampledQ             ; clock pipe for edge detection
        movf    GPIO, W                   ; fresh sample
        movwf   GPIO_sampled              ; sampled copy of IO's
; check for rising clock edge
        btfss   GPIO_sampled, GP0
        goto    sampler                   ; if not set, then it's not rising
        btfsc   GPIO_sampledQ, GP0        ; if old is clear, then rising
        goto    sampler

shifter:
        movf    GPIO, W                   ; sample the data
        movwf   GPIO_sampledQ             ; reuse the same memory location
        btfss   GPIO_sampledQ, GP0
        goto    sampler                   ; if clock has already dropped, abort
        bsf     STATUS, C
        btfss   GPIO_sampledQ, GP1        ; sets C to match GP1 for shift in
        bcf     STATUS, C
        rrf     magicKeyH                 ; first half of..
        rrf     magicKeyL                 ; 16-bit shift through C
        movlw   0xac
        subwf   magicKeyH, W
        btfss   STATUS, Z                 ; check for key-pattern match
        goto    sampler
        movlw   0x5a
        subwf   magicKeyL, W
        btfss   STATUS, Z                 ; check for key-pattern match
        goto    sampler
erase:
; begin erase trigger
        bsf     GPIO, GP2
; delay for erase trigger period, estimate 393mS for this code...
delay:
        clrf    tick0
        clrf    tick1
delayLoop:
; inc16
        incf    tick0
        btfsc   STATUS, Z
        incf    tick1
        btfss   STATUS, Z
        goto    delayLoop
; end erase trigger
        bcf     GPIO, GP2
        goto    sampler
    end
