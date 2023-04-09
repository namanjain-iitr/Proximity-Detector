import React from 'react';
import { Grid, Row, Col } from 'react-flexbox-grid';
import './gridList.css'

const GridList = ({ data }) => {
  return (
    <Grid fluid>
    {data? data.map(item => (
      <div className="grid-item" key={item.id}>
        <div className="row">
          <div className="name">{item.name}</div>
          <div className="last-seen">Last seen : <div className = "time-stamp">{item.lastSeen} </div> </div>
        </div>
        <div className="row">
          <div className="mac-address">MAC Address : {item.macAddress}</div>
          <div className="rssi">RSSI : {item.rssi} dBm</div>
        </div>
      </div>
    )): <div></div>}
    </Grid>
  );
};

export default GridList;
