// `rustc primes.rs -O (`rustc -O` = `gcc -O 2`)
// finds all primes ≤ 2**32 in ~2 minutes w/ 1 thread
#![feature(i128_type)]

const SIZE: usize = 33_554_432;   // 2**32 / 128

struct Sieve(Vec<u128>); // of Eratosthenes

impl Sieve {
    fn new() -> Self {
        Sieve((0..SIZE).map(|_| 0).collect())
    }
    fn get(&self, bit: u32) -> bool {
        let (index, offset) = (bit/128, bit%128);
        (self.0[index as usize] & (1 << offset)) != 0
    }
    fn set(&mut self, bit: u32) {
        let (index, offset) = (bit/128, bit%128);
        self.0[index as usize] |= 1 << offset;
    }
}

fn main() {
    let mut s = Sieve::new();
    let mut j;
    let mut top = 10u32;
    let mut count = 0u32;
    println!("\t\tN\t\tPrimes");

    for i in 2 .. u32::max_value() {
        if i == top {
            println!("\t\t{}\t\t{}", top, count);
            if top < u32::max_value() / 10 {
                top *= 10;
            } else {
                top = u32::max_value()-1;
            }
        }
        if s.get(i) == false {
            count += 1;
            j = i;
            while j < u32::max_value() - i {
                j += i;
                s.set(j);
            }
        }
    }
}
