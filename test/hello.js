function makeFriendly(value) {
  return typeof value === 'string' ? value : JSON.stringify(value, null, 2)
}

function log(...args) {
  print(...args.map(makeFriendly))
}

async function main () {
  log('started')

  log({ cwd })

  log(constants)

  print('wat')
  const [ readFd, writeFd ] = await Promise.all([
    open(cwd + '/test/hello.js', constants.RDONLY),
    open(cwd + '/test/hello2.js', constants.WRONLY, constants.CREAT)
  ])
  try {
    const buf = await read(readFd, 1024)
    const res = await write(writeFd, buf)
    print({ res })
  } catch (err) {
    print(err.stack)
  } finally {
    await Promise.all([
      close(readFd),
      close(writeFd)
    ])
    log('closed the files')
  }
}

main()
