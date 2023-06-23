const crypto = require("node:crypto")

const uvc = require('./uvc');

console.log("Hello world")

uvc.uvCheck();

uvc.initUvCheck();

const TIMEOUT = 250;
const LOOPS = 3;
let count = 0;

const f = () => {
    for (let i = 0; i < 5; i++) {
        let b = Buffer.alloc(2**20, 0)
        crypto.randomFill(b, (err, _) => {
            if (err) throw err;
        })
    }

    console.log('')
    const x = uvc.getUvCheckInfo();
    console.log('x:', x)

    count += 1;
    if (count < LOOPS) {
        setTimeout(f, TIMEOUT);
    } else {
        setTimeout(() => console.log(uvc.getUvCheckInfo()), TIMEOUT)
        setTimeout(() => uvc.stopUvCheck(), TIMEOUT * 2)
    }
}

setTimeout(f, TIMEOUT);
