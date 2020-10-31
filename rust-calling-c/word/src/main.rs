mod word;
use std::ffi::CString;
use std::ffi::CStr;

fn main()
{
    /* Create a CString */
    let cstr = CString::new("Cisco!").expect("CString::new() failed");
    let w_info = word::word_info_t
    {
        word: cstr.as_ptr(),
        validity: false,
    };

    unsafe
    {   
        let cstr2: &CStr = CStr::from_ptr(word::get_word(&w_info));
        println!("word: {:?}, validity: {:?}", cstr2, word::get_validity(&w_info));
    };
}
