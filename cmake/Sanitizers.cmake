include_guard(GLOBAL)

option(TATITO_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(TATITO_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

function(tatito_enable_sanitizers target)
    if(NOT TATITO_ENABLE_ASAN AND NOT TATITO_ENABLE_UBSAN)
        return()
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(WARNING "Sanitizers are not configured for ${CMAKE_CXX_COMPILER_ID}")
        return()
    endif()

    set(enabled_sanitizers)
    if(TATITO_ENABLE_ASAN)
        list(APPEND enabled_sanitizers address)
    endif()
    if(TATITO_ENABLE_UBSAN)
        list(APPEND enabled_sanitizers undefined)
    endif()

    list(JOIN enabled_sanitizers "," sanitizer_list)
    set(sanitizer_flag "-fsanitize=${sanitizer_list}")

    target_compile_options(${target} PRIVATE
            ${sanitizer_flag}
            -fno-omit-frame-pointer
            -fno-sanitize-recover=all
    )
    target_link_options(${target} PRIVATE ${sanitizer_flag})
endfunction()
