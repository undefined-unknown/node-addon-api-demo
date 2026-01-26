#!/usr/bin/env node

// 文件级关闭
/* eslint-disable */

/**
 * 从 CHANGELOG.md 中提取指定版本的更新日志
 * 用于在构建时注入到 electron-builder 的 releaseNotes 中
 *
 * 使用方法：
 * node scripts/extract-changelog.js [版本号]
 *
 * 示例：
 * node scripts/extract-changelog.js 1.0.1
 */

const fs = require('fs')
const path = require('path')

// 获取版本号（从参数或 package.json）
const version = process.argv[2] || require('../package.json').version
const changelogPath = path.join(__dirname, '..', 'CHANGELOG.md')

if (!fs.existsSync(changelogPath)) {
  console.error('❌ CHANGELOG.md 文件不存在')
  process.exit(1)
}

const changelog = fs.readFileSync(changelogPath, 'utf-8')

// 匹配指定版本的内容（从版本标题到下一个版本标题或文件结尾）
const versionRegex = new RegExp(`## \\[${version.replace(/\./g, '\\.')}\\][\\s\\S]*?(?=## \\[|$)`, 'i')

const match = changelog.match(versionRegex)

if (!match) {
  console.error(`❌ 未找到版本 ${version} 的更新日志`)
  process.exit(1)
}

let releaseNotes = match[0]
  // 移除版本标题行
  .replace(/^## \[.*?\] - .*?\n/, '')
  // 移除分类标题（新增、修复等），保留内容
  .replace(/^### (新增|修复|优化|变更|废弃|移除|安全)$/gm, '**$1**')
  // 清理多余空行
  .replace(/\n{3,}/g, '\n\n')
  .trim()

if (!releaseNotes) {
  console.error(`❌ 版本 ${version} 的更新日志为空`)
  process.exit(1)
}

// 输出到 stdout（可以被其他脚本使用）
console.log(releaseNotes)
