fn main ()
{
    let mut arr: [i64; 100] = [0; 100];
    dummy(&mut arr);
}

fn dummy (arr: &mut [i64; 100])
{   
    for val in arr.iter()
    {
        println!("{}", val);
    }
}
