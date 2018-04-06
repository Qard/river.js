async function main () {
  try {
    const [ readFd, writeFd ] = await Promise.all([
      fs.open(cwd + '/test/hello.js', fs.constants.RDONLY),
      fs.open(cwd + '/test/hello2.js', fs.constants.WRONLY | fs.constants.CREAT, 0o744)
    ])

    try {
      const buf = await fs.read(readFd, 1024)
      const res = await fs.write(writeFd, buf)
    } finally {
      await Promise.all([
        fs.close(readFd),
        fs.close(writeFd)
      ])
    }
  } catch(err) {
    console.error(err.stack)
  }
}

main().then(
  print.bind(null, 'success'),
  print.bind(null, 'failure')
)
