const crypto = require("node:crypto")

const uvc = require('./uvc');

uvc.uvCheck();

uvc.initUvCheck();

const JOBS = 500
const CHECKPOINT = JOBS / 4
var next_checkpoint = JOBS - CHECKPOINT

console.log("Starting..")
const start = process.hrtime.bigint()

for (let i = 0; i < JOBS; i++) {
    crypto.scrypt('asdjfkaljsdfkljasdklfjasdklfjasdklfjskladfjklsdafjklasdfjksldafjaskldfjksdlfjklasfjklsdjflkasdjfklsdajflksdjfklsd', 'salt', 64, (err, ) => {
        if (err) throw err;
    })
}

console.log("Kicked off all jobs. #:", uvc.getUvCheckInfo().activeReqs)

const check = () => {
    const info = uvc.getUvCheckInfo()

    if (info.activeReqs <= next_checkpoint) {
        console.log(info)
        next_checkpoint -= CHECKPOINT
    }

    if (info.activeReqs == 0) {
        console.log("Done. Took:", (process.hrtime.bigint() - start) / BigInt(1e6), "milliseconds")
        uvc.stopUvCheck()
    } else {
        setTimeout(() => check())
    }
}

check()
