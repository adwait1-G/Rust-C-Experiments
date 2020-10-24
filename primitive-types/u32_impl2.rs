fn main()
{
    let mut arr: [u8; 4] = [0x12, 0x34, 0x56, 0x78];
    from_ne_bytes(arr);
}

fn from_ne_bytes(mut arr: [u8; 4])
{
    let mut val:u32 = u32::from_ne_bytes(arr);
    dummy(val);
}


fn dummy(mut x: u32)
{
    println!("{:X}", x);
}

