import GridList from './gridList.js'
import MacAddressForm from './macAddressForm.js'
import './App.css';
import React, {Component} from 'react';
import axios from 'axios'

// var macData = [
//   {
//     "id" : 1,
//     "name" : "Ayush",
//     "macAddress" : "34:124:21:45",
//     "lastSeen" : "4:00:00",
//     "rssi" : "-45"
//   },
//   {
//     "id" : 2,
//     "name" : "Ayush",
//     "macAddress" : "34:124:21:45",
//     "lastSeen" : "4:00:00",
//     "rssi" : "-45"
//   }
// ]

class App extends Component {

  constructor(props) {
    super(props);
    this.state = {
      macData : [],
    }
  }

handleAddMacAddress = (mac, name) => {
    let payLoad = {
      "macAddress" : mac,
      "name" : name
    }
    console.log(payLoad);
    axios.post('/add-macAddress', payLoad)
    .then(res => console.log(res));

}
async componentDidMount() {
  axios.get('/fetch-mac-data')
  .then(res => {
    console.log(res);
    this.setState({macData : res.data});
  })
  .catch();
}

  render() {
    const {macData} = this.state;
    console.log(macData);
    return (
      <div className="App">
        <header className="App-header">
        <h1> Proximity Detector </h1>
          <MacAddressForm onAddMacAddress = {this.handleAddMacAddress}/>
          <GridList data = {macData}/>
        </header>
      </div>
    );
  }
}

export default App;
