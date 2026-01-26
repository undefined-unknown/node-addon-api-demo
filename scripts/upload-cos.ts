import COS from 'cos-nodejs-sdk-v5'
import fs from 'fs'
import path from 'path'
import { fileURLToPath } from 'url'
import dotenv from 'dotenv'

// 获取 __dirname (ES 模块兼容)
const __filename = fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

dotenv.config({ path: path.join(__dirname, '../.env') })

// 配置
const distDir = path.join(__dirname, '../dist')
const bucket = 'download-1316861839'
const region = 'ap-guangzhou'
const prefix = 'Studio/' // 修正：从 Studios/ 改为 Studio/，与 electron-builder.yml 一致

// 读取 package.json 获取版本号
const packageJson = JSON.parse(fs.readFileSync(path.join(__dirname, '../package.json'), 'utf-8'))
const currentVersion = packageJson.version

interface UploadFile {
  filePath: string
  fileName: string
  key: string
  isYml: boolean
  size: number
}

/**
 * 检查环境变量
 */
function checkEnv(): void {
  if (!process.env.TENCENT_SECRET_ID) {
    throw new Error('❌ TENCENT_SECRET_ID 环境变量未设置')
  }
  if (!process.env.TENCENT_SECRET_KEY) {
    throw new Error('❌ TENCENT_SECRET_KEY 环境变量未设置')
  }
}

/**
 * 检查 dist 目录是否存在
 */
function checkDistDir(): void {
  if (!fs.existsSync(distDir)) {
    throw new Error(`❌ dist 目录不存在: ${distDir}`)
  }
}

/**
 * 递归读取目录中的所有文件
 */
function getAllFiles(dir: string, fileList: string[] = []): string[] {
  if (!fs.existsSync(dir)) {
    return fileList
  }

  const files = fs.readdirSync(dir)

  files.forEach((file) => {
    const filePath = path.join(dir, file)
    const stat = fs.statSync(filePath)

    if (stat.isDirectory()) {
      getAllFiles(filePath, fileList)
    } else {
      fileList.push(filePath)
    }
  })

  return fileList
}

/**
 * 检查文件是否属于当前版本
 */
function isCurrentVersionFile(fileName: string): boolean {
  // latest.yml 或 latest-*.yml 文件总是需要上传（它们是更新清单文件）
  // Windows 可能生成 latest.yml，macOS 生成 latest-mac.yml
  if (fileName === 'latest.yml' || (fileName.startsWith('latest-') && fileName.endsWith('.yml'))) {
    return true
  }

  // 检查文件名是否包含当前版本号
  // 格式: ClawLabStudio-mac-1.0.1-arm64.zip
  // 或: ClawLabStudio-win-1.0.1-x64.exe
  const versionPattern = new RegExp(`-${currentVersion.replace(/\./g, '\\.')}-`, 'i')
  return versionPattern.test(fileName)
}

/**
 * 获取需要上传的文件列表（只包含当前版本和更新清单文件）
 */
function getUploadFiles(): UploadFile[] {
  const allFiles = getAllFiles(distDir)
  const uploadFiles: UploadFile[] = []

  // 自动更新必需的文件扩展名
  const requiredExts = ['.yml', '.yaml'] // 更新清单文件
  const updateExts = ['.zip', '.dmg', '.exe', '.blockmap', '.AppImage', '.deb', '.snap'] // 更新包文件
  const allowedExts = [...requiredExts, ...updateExts]

  allFiles.forEach((filePath) => {
    const fileName = path.basename(filePath)
    const ext = path.extname(fileName).toLowerCase()
    const relativePath = path.relative(distDir, filePath)
    const key = prefix + relativePath.replace(/\\/g, '/') // 统一使用 / 作为路径分隔符

    // 只处理允许的扩展名
    if (!allowedExts.includes(ext)) {
      return
    }

    // 检查是否是当前版本的文件或更新清单文件
    if (!isCurrentVersionFile(fileName)) {
      console.log(`⏭️  跳过旧版本文件: ${fileName}`)
      return
    }

    const fileStats = fs.statSync(filePath)
    uploadFiles.push({
      filePath,
      fileName,
      key,
      isYml: requiredExts.includes(ext),
      size: fileStats.size
    })
  })

  return uploadFiles
}

/**
 * 上传单个文件到 COS
 */
async function uploadFile(cos: COS, file: UploadFile): Promise<void> {
  return new Promise((resolve, reject) => {
    if (!fs.existsSync(file.filePath)) {
      reject(new Error(`文件不存在: ${file.filePath}`))
      return
    }

    const fileSizeMB = (file.size / 1024 / 1024).toFixed(2)

    console.log(`📤 上传: ${file.fileName} (${fileSizeMB} MB)`)

    const headers: Record<string, string> = {
      // 防止 CDN/浏览器缓存旧的包体导致校验和不一致
      'Cache-Control': 'no-cache, no-store, must-revalidate',
      Pragma: 'no-cache',
      Expires: '0'
    }

    // YML 文件设置特殊 Content-Type 和缓存策略
    if (file.isYml) {
      headers['Content-Type'] = 'text/yaml; charset=utf-8'
      // 更新清单文件可以缓存，但时间较短（5分钟）
      headers['Cache-Control'] = 'public, max-age=300'
    } else {
      // 更新包文件不缓存
      headers['Cache-Control'] = 'no-cache, no-store, must-revalidate'
    }

    cos.putObject(
      {
        Bucket: bucket,
        Region: region,
        Key: file.key,
        Body: fs.createReadStream(file.filePath), // 使用流式上传，避免大文件内存问题
        Headers: headers
      },
      (err, data) => {
        if (err) {
          console.error(`❌ 上传失败 ${file.fileName}:`, err.message)
          reject(err)
        } else {
          const url = `https://${data.Location}`
          console.log(`✅ 上传成功: ${file.fileName}`)
          console.log(`   URL: ${url}`)
          resolve()
        }
      }
    )
  })
}

/**
 * 主函数
 */
async function main(): Promise<void> {
  try {
    console.log('🚀 开始上传自动更新文件到 COS...\n')
    console.log(`📦 当前版本: ${currentVersion}\n`)

    // 检查环境变量
    checkEnv()
    console.log('✅ 环境变量检查通过')

    // 检查 dist 目录
    checkDistDir()
    console.log(`✅ dist 目录存在: ${distDir}\n`)

    // 获取需要上传的文件（只包含当前版本）
    const uploadFiles = getUploadFiles()

    if (uploadFiles.length === 0) {
      console.log('⚠️  没有找到需要上传的文件')
      console.log(`   提示: 请确保已构建应用 (yarn build:mac 或 yarn build:win)`)
      console.log(`   当前版本: ${currentVersion}`)
      return
    }

    // 分类显示文件
    const ymlFiles = uploadFiles.filter((f) => f.isYml)
    const updateFiles = uploadFiles.filter((f) => !f.isYml)

    console.log(`📋 找到 ${uploadFiles.length} 个文件需要上传:\n`)

    if (ymlFiles.length > 0) {
      console.log('📄 更新清单文件:')
      ymlFiles.forEach((file) => {
        console.log(`   - ${file.fileName}`)
      })
      console.log('')
    }

    if (updateFiles.length > 0) {
      console.log('📦 更新包文件:')
      updateFiles.forEach((file) => {
        const sizeMB = (file.size / 1024 / 1024).toFixed(2)
        console.log(`   - ${file.fileName} (${sizeMB} MB)`)
      })
      console.log('')
    }

    // 验证必需文件
    const hasLatestMac = ymlFiles.some((f) => f.fileName.includes('latest-mac'))
    // Windows 可能生成 latest.yml 或 latest-win.yml
    const hasLatestWin = ymlFiles.some((f) => f.fileName === 'latest.yml' || f.fileName.includes('latest-win'))
    const hasWinExe = updateFiles.some((f) => f.fileName.endsWith('.exe'))
    const hasMacZip = updateFiles.some((f) => f.fileName.endsWith('.zip'))
    const hasUpdatePackage = updateFiles.length > 0

    // Windows 特定验证
    if (hasLatestWin && !hasWinExe) {
      const latestWinFile = ymlFiles.find((f) => f.fileName === 'latest.yml' || f.fileName.includes('latest-win'))
      console.warn(`⚠️  警告: 找到 ${latestWinFile?.fileName || 'latest.yml'} 但未找到对应的 .exe 文件`)
      console.warn('   Windows 自动更新需要 .exe 安装包文件\n')
    } else if (hasWinExe && !hasLatestWin) {
      console.warn('⚠️  警告: 找到 .exe 文件但未找到 latest.yml 或 latest-win.yml 更新清单文件')
      console.warn('   Windows 自动更新需要 latest.yml 或 latest-win.yml 文件\n')
    }

    // macOS 特定验证
    if (hasLatestMac && !hasMacZip) {
      console.warn('⚠️  警告: 找到 latest-mac.yml 但未找到对应的 .zip 文件')
      console.warn('   macOS 自动更新需要 .zip 增量更新包文件\n')
    } else if (hasMacZip && !hasLatestMac) {
      console.warn('⚠️  警告: 找到 .zip 文件但未找到 latest-mac.yml 更新清单文件')
      console.warn('   macOS 自动更新需要 latest-mac.yml 文件\n')
    }

    // 通用验证
    if (!hasLatestMac && !hasLatestWin) {
      console.warn('⚠️  警告: 未找到 latest-*.yml 或 latest.yml 更新清单文件')
      console.warn('   这可能导致自动更新无法正常工作\n')
    }

    if (!hasUpdatePackage) {
      console.warn('⚠️  警告: 未找到更新包文件 (.zip, .dmg, .exe 等)')
      console.warn('   这可能导致自动更新无法正常工作\n')
    }

    // 初始化 COS 客户端
    const cos = new COS({
      SecretId: process.env.TENCENT_SECRET_ID!,
      SecretKey: process.env.TENCENT_SECRET_KEY!
    })

    // 先上传更新清单文件，再上传更新包
    const sortedFiles = [...ymlFiles, ...updateFiles]

    // 并发上传所有文件（但等待所有完成）
    const uploadPromises = sortedFiles.map((file) =>
      uploadFile(cos, file).catch((err) => {
        console.error(`❌ ${file.fileName} 上传失败:`, err.message)
        throw err
      })
    )

    await Promise.all(uploadPromises)

    console.log('\n✨ 所有文件上传完成！')
    console.log(`📦 上传了 ${uploadFiles.length} 个文件`)
    console.log(`🌐 访问地址: https://download.clawlab.cn/${prefix}`)
    console.log(`\n📝 提示:`)
    console.log(`   - 更新清单文件: latest-*.yml`)
    console.log(`   - 更新包文件: 包含版本号 ${currentVersion} 的文件`)
    console.log(`   - 旧版本文件已自动跳过，不会重复上传`)
  } catch (error) {
    console.error('\n❌ 上传过程中发生错误:')
    console.error(error instanceof Error ? error.message : error)
    process.exit(1)
  }
}

// 执行主函数
main()
