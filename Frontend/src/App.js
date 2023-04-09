import GridList from './gridList.js'
import MacAddressForm from './macAddressForm.js'
import './App.css';
import React, { useState, useEffect } from 'react';
import axios from 'axios'

function App() {
  const [data, setData] = useState(null);

  useEffect(() => {
    fetchData(); // initial fetch
    const intervalId = setInterval(fetchData, 10000); // fetch every 5 minutes
    return () => clearInterval(intervalId); // cleanup
  }, []);

  const fetchData = async () => {
    try {
      const response = await fetch('/fetch-mac-data');
      const macData = await response.json();
      console.log(macData);
      setData(macData);
    } catch (error) {
      console.log("error");
      console.error(error);
    }
  };

  let handleAddMacAddress = (mac, name) => {
    let payLoad = {
      "macAddress" : mac,
      "name" : name
    }
    console.log(payLoad);
    axios.post('/add-macAddress', payLoad)
    .then(res => console.log(res));
}
  return (
    <div className="App">
        <header className="App-header">
        <h1> Proximity Detector </h1>
          <MacAddressForm onAddMacAddress = {handleAddMacAddress}/>
          <GridList data = {data}/>
        </header>
      </div>
  );
}

export default App;
