import Versions from './components/Versions'
import electronLogo from './assets/electron.svg'

function App(): React.JSX.Element {
  const ipcHandle = (): void => window.electron.ipcRenderer.send('ping')

  const handleGetHello = async (): Promise<void> => {
    const result = await window.electron.ipcRenderer.invoke('addon:getHello')
    console.log(result)
  }

  const handleAdd = async (): Promise<void> => {
    const result = await window.electron.ipcRenderer.invoke('addon:add', 1, 2)
    console.log(result)
  }
  return (
    <>
      <button onClick={handleGetHello}>getHello</button>
      <button onClick={handleAdd}>add</button>
      <img alt="logo" className="logo" src={electronLogo} />
      <div className="creator">Powered by electron-vite</div>
      <div className="text">
        Build an Electron app with <span className="react">React</span>
        &nbsp;and <span className="ts">TypeScript</span>
      </div>
      <p className="tip">
        Please try pressing <code>F12</code> to open the devTool
      </p>
      <div className="actions">
        <div className="action">
          <a href="https://electron-vite.org/" target="_blank" rel="noreferrer">
            Documentation
          </a>
        </div>
        <div className="action">
          <a target="_blank" rel="noreferrer" onClick={ipcHandle}>
            Send IPC
          </a>
        </div>
      </div>
      <Versions></Versions>
    </>
  )
}

export default App
