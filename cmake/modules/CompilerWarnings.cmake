include(CheckCXXCompilerFlag)
include(CheckCCompilerFlag)

macro(check_lang_compiler_flag lang flag variable)

    if(${lang} STREQUAL c)
        check_c_compiler_flag(${flag} ${variable})
    endif()

    if(${lang} STREQUAL cxx)
        check_cxx_compiler_flag(${flag} ${variable})
    endif()

endmacro()

macro(compiler_warnings ret lang werror_by_default desirable_flags undesirable_flags)
    set(warning_flags "")
    foreach(flag ${desirable_flags})
        check_lang_compiler_flag(${lang} -W${flag} ${flag}_${lang}_result)
        if(${${flag}_${lang}_result})
            set(warning_flags "${warning_flags} -W${flag}")
        endif()
    endforeach()

    check_lang_compiler_flag(${lang} -Werror error_${lang}_result)

    if(${error_${lang}_result})
        set(error_flags "-Werror")
    endif()

    set(all_nowarning_flags_supported 1)

    foreach(flag ${undesirable_flags})
        check_lang_compiler_flag(${lang} -Wno-${flag} ${flag}_${lang}_result)

        if(${${flag}_${lang}_result})
            set(warning_flags "${warning_flags} -Wno-${flag}")
        else()
            set(all_nowarning_flags_supported 0)
            break()
        endif()

        check_lang_compiler_flag(${lang} -Wno-error=${flag} noerror_${flag}_${lang}_result)

        if(${noerror_${flag}_${lang}_result})
            set(error_flags "${error_flags} -Wno-error=${flag}")
        endif()

    endforeach()

    if(DISABLE_WERROR)
        set(enable_werror 0)
    else()
        set(enable_werror 1)
    endif()

    if(${werror_by_default} AND ${enable_werror} AND ${all_nowarning_flags_supported})
        set(${ret} "${warning_flags} ${error_flags}")
    else()
        set(${ret} "${warning_flags}")
    endif()

endmacro()
