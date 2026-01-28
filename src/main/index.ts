import { app, shell, BrowserWindow, ipcMain, globalShortcut } from 'electron'
import { join } from 'path'
import fs from 'fs'
import { electronApp, optimizer, is } from '@electron-toolkit/utils'
import icon from '../../resources/icon.png?asset'
import bindings from 'bindings'

const addon = bindings('yima_addon')

function createWindow(): void {
  // Create the browser window.
  const mainWindow = new BrowserWindow({
    width: 900,
    height: 670,
    show: false,
    autoHideMenuBar: true,
    ...(process.platform === 'linux' ? { icon } : {}),
    webPreferences: {
      preload: join(__dirname, '../preload/index.js'),
      sandbox: false
    }
  })

  mainWindow.on('ready-to-show', () => {
    mainWindow.show()
  })

  mainWindow.webContents.setWindowOpenHandler((details) => {
    shell.openExternal(details.url)
    return { action: 'deny' }
  })

  // HMR for renderer base on electron-vite cli.
  // Load the remote URL for development or the local html file for production.
  if (is.dev && process.env['ELECTRON_RENDERER_URL']) {
    mainWindow.loadURL(process.env['ELECTRON_RENDERER_URL'])
  } else {
    mainWindow.loadFile(join(__dirname, '../renderer/index.html'))
  }
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(() => {
  // Set app user model id for windows
  electronApp.setAppUserModelId('com.electron')

  // 注册全局快捷键：Cmd+Shift+D (macOS) 或 Ctrl+Shift+D (Windows/Linux) 打开开发者工具
  const devToolsShortcut = process.platform === 'darwin' ? 'Command+Shift+D' : 'Ctrl+Shift+D'
  globalShortcut.register(devToolsShortcut, () => {
    const mainWindow = BrowserWindow.getAllWindows().find((window) => !window.isDestroyed())
    if (mainWindow) {
      mainWindow.webContents.toggleDevTools()
    }
  })

  // Default open or close DevTools by F12 in development
  // and ignore CommandOrControl + R in production.
  // see https://github.com/alex8088/electron-toolkit/tree/master/packages/utils
  app.on('browser-window-created', (_, window) => {
    optimizer.watchWindowShortcuts(window)
  })

  ipcMain.handle('addon:getHello', () => {
    // 在开发环境和生产环境中正确获取资源路径
    let configPath: string
    let inputPath: string
    let outputPath: string

    if (is.dev) {
      // 开发环境：使用项目根目录下的 resources
      // 使用 app.getAppPath() 获取应用根目录，确保中文路径正确处理
      const appPath = app.getAppPath()
      configPath = join(appPath, 'resources/config')
      inputPath = join(appPath, 'resources/input')
      outputPath = join(appPath, 'resources/output')
    } else {
      // 生产环境：
      // - config 从 extraResources 中读取（只读）
      // - input/output 放在用户数据目录中（可写）
      configPath = join(process.resourcesPath, 'resources/config')
      inputPath = join(process.resourcesPath, 'resources/input')
      outputPath = join(process.resourcesPath, 'resources/output')
    }

    // 确保目录存在
    if (!fs.existsSync(inputPath)) fs.mkdirSync(inputPath, { recursive: true })
    if (!fs.existsSync(outputPath)) fs.mkdirSync(outputPath, { recursive: true })

    try {
      // 调用 Addon
      console.log('\nCalling ProcessBmpTranslation...')
      console.log(`Config path: ${configPath}`)
      console.log(`Input path: ${inputPath}`)
      console.log(`Output path: ${outputPath}`)
      const result = addon.processBmpTranslation(configPath, inputPath, outputPath)

      console.log('\n--------------------------')
      if (result === 0) {
        console.log('✅ Success! Result code: 0')
      } else {
        console.log(`❌ Failed! Result code: ${result}`)
        console.log('Check the console output for error details.')
      }
      return result
    } catch (error) {
      console.error('❌ Exception:', error)
      return -1
    }
  })

  ipcMain.handle('addon:add', (_event, a: number, b: number) => addon.add(a, b))

  // IPC test
  ipcMain.on('ping', () => console.log('pong'))

  createWindow()

  app.on('activate', function () {
    // On macOS it's common to re-create a window in the app when the
    // dock icon is clicked and there are no other windows open.
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them here.
