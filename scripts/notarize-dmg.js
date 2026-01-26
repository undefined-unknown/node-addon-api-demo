// 文件级关闭
/* eslint-disable */

require('dotenv').config()
const { notarize } = require('@electron/notarize')
const path = require('path')
const fs = require('fs')

// 从 package.json 读取当前版本号
function getCurrentVersion() {
  const packageJsonPath = path.resolve('./package.json')
  if (!fs.existsSync(packageJsonPath)) {
    throw new Error('package.json not found')
  }
  const packageJson = JSON.parse(fs.readFileSync(packageJsonPath, 'utf-8'))
  return packageJson.version
}

// 检查文件名是否包含当前版本号
function isCurrentVersionFile(fileName, currentVersion) {
  // 将版本号中的点转义，用于正则表达式
  const escapedVersion = currentVersion.replace(/\./g, '\\.')
  // 检查文件名是否包含当前版本号
  const versionRegex = new RegExp(escapedVersion)
  return versionRegex.test(fileName)
}

// 公证 DMG 文件
// 可以作为独立脚本运行，也可以在 afterAllArtifactBuild hook 中调用
async function notarizeDMGFiles() {
  const outDir = path.resolve('./dist')

  console.log(`📂 Output directory: ${outDir}`)

  if (process.platform !== 'darwin') {
    console.log('⏭️  Skipping DMG notarization (not macOS)')
    return
  }

  // 确保环境变量已加载
  if (!process.env.APPLE_TEAM_ID || !process.env.APPLE_ID || !process.env.APPLE_APP_SPECIFIC_PASSWORD) {
    console.error('❌ Missing Apple notarization credentials in environment variables')
    console.error('Required: APPLE_TEAM_ID, APPLE_ID, APPLE_APP_SPECIFIC_PASSWORD')
    return
  }

  // 获取当前版本号
  let currentVersion
  try {
    currentVersion = getCurrentVersion()
    console.log(`📌 Current version: ${currentVersion}`)
  } catch (error) {
    console.error('❌ Failed to read version from package.json:', error.message)
    return
  }

  // 查找所有 DMG 文件
  const distDir = path.resolve(outDir)
  if (!fs.existsSync(distDir)) {
    console.warn(`⚠️  Output directory does not exist: ${distDir}`)
    return
  }

  const allDmgFiles = fs.readdirSync(distDir).filter((file) => file.endsWith('.dmg'))

  if (allDmgFiles.length === 0) {
    console.warn('⚠️  No DMG files found in dist directory')
    return
  }

  // 过滤出当前版本的 DMG 文件
  const currentVersionDmgFiles = allDmgFiles.filter((file) => isCurrentVersionFile(file, currentVersion))
  const oldVersionDmgFiles = allDmgFiles.filter((file) => !isCurrentVersionFile(file, currentVersion))

  if (oldVersionDmgFiles.length > 0) {
    console.log(`⏭️  Skipping ${oldVersionDmgFiles.length} old version DMG file(s):`)
    oldVersionDmgFiles.forEach((file) => {
      console.log(`   - ${file}`)
    })
  }

  if (currentVersionDmgFiles.length === 0) {
    console.warn(`⚠️  No DMG files found for current version (${currentVersion}) to notarize`)
    return
  }

  console.log(`📦 Found ${currentVersionDmgFiles.length} DMG file(s) for current version to notarize`)

  for (const dmgFile of currentVersionDmgFiles) {
    const dmgPath = path.join(distDir, dmgFile)
    console.log(`📦 Notarizing DMG: ${dmgFile}`)

    try {
      await notarize({
        tool: 'notarytool',
        appBundleId: 'cn.clawlab.studio',
        appPath: dmgPath,
        teamId: process.env.APPLE_TEAM_ID,
        appleId: process.env.APPLE_ID,
        appleIdPassword: process.env.APPLE_APP_SPECIFIC_PASSWORD
      })

      console.log(`✅ DMG notarized successfully: ${dmgFile}`)

      // 附加公证票据（staple）
      const { execSync } = require('child_process')
      try {
        execSync(`xcrun stapler staple "${dmgPath}"`, { stdio: 'inherit' })
        console.log(`✅ Staple attached to DMG: ${dmgFile}`)
      } catch (stapleError) {
        console.warn(`⚠️  Failed to staple DMG (may need to wait for notarization to complete): ${stapleError.message}`)
      }
    } catch (error) {
      console.error(`❌ Failed to notarize DMG ${dmgFile}:`, error)
      throw error
    }
  }
}

// 如果作为独立脚本运行
if (require.main === module) {
  notarizeDMGFiles().catch((error) => {
    console.error('❌ DMG notarization failed:', error)
    process.exit(1)
  })
}

// 如果作为 hook 调用
exports.default = async function notarizeDMG(context) {
  console.log('🔍 afterAllArtifactBuild hook called')

  const outDir = context?.outDir || context?.outputDirectory || './dist'
  const electronPlatformName = context?.electronPlatformName || context?.platform?.name || 'darwin'

  if (electronPlatformName !== 'darwin') {
    console.log('⏭️  Skipping DMG notarization (not macOS)')
    return
  }

  // 使用相同的逻辑
  await notarizeDMGFiles()
}
