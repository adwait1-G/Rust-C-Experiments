fn main()
{
    let tuple = ("Hello", 5, 'c');
    dummy(&tuple);
}

fn dummy(tuple: &(&str, i32, char))
{
    println!("({}, {}, {})", tuple.0, tuple.1, tuple.2);        
}
